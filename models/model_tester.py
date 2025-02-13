import tensorrt as trt
import numpy as np
import os

import pycuda.driver as cuda
import pycuda.autoinit

from torchvision import models, transforms
from PIL import Image
import plotly.express as px
import numpy as np

import cv2

import plotly.graph_objects as go
from plotly.subplots import make_subplots
import matplotlib.pyplot as plt



class HostDeviceMem(object):
    def __init__(self, host_mem, device_mem):
        self.host = host_mem
        self.device = device_mem

    def __str__(self):
        return "Host:\n" + str(self.host) + "\nDevice:\n" + str(self.device)

    def __repr__(self):
        return self.__str__()

class TrtModel:
    
    def __init__(self,engine_path,max_batch_size=1,dtype=np.float32):
        
        self.engine_path = engine_path
        self.dtype = dtype
        self.logger = trt.Logger(trt.Logger.WARNING)
        self.runtime = trt.Runtime(self.logger)
        self.engine = self.load_engine(self.runtime, self.engine_path)
        self.max_batch_size = max_batch_size
        self.inputs, self.outputs, self.bindings, self.stream = self.allocate_buffers()
        self.context = self.engine.create_execution_context()

                
                
    @staticmethod
    def load_engine(trt_runtime, engine_path):
        trt.init_libnvinfer_plugins(None, "")             
        with open(engine_path, 'rb') as f:
            engine_data = f.read()
        engine = trt_runtime.deserialize_cuda_engine(engine_data)
        return engine
    
    def allocate_buffers(self):
        
        inputs = []
        outputs = []
        bindings = []
        stream = cuda.Stream()
        
        for binding in self.engine:
            size = trt.volume(self.engine.get_binding_shape(binding)) * self.max_batch_size
            host_mem = cuda.pagelocked_empty(size, self.dtype)
            device_mem = cuda.mem_alloc(host_mem.nbytes)
            
            bindings.append(int(device_mem))

            if self.engine.binding_is_input(binding):
                inputs.append(HostDeviceMem(host_mem, device_mem))
            else:
                outputs.append(HostDeviceMem(host_mem, device_mem))
        
        return inputs, outputs, bindings, stream
       
            
    def __call__(self,x:np.ndarray,batch_size=2):
        
        x = x.astype(self.dtype)
        
        np.copyto(self.inputs[0].host,x.ravel())
        
        for inp in self.inputs:
            cuda.memcpy_htod_async(inp.device, inp.host, self.stream)
        
        self.context.execute_async(batch_size=batch_size, bindings=self.bindings, stream_handle=self.stream.handle)
        for out in self.outputs:
            cuda.memcpy_dtoh_async(out.host, out.device, self.stream) 
            
        
        self.stream.synchronize()
        return [out.host.reshape(batch_size,-1) for out in self.outputs]


        
        
if __name__ == "__main__":
 
    batch_size = 1
    trt_engine_path = os.path.join(".","smoke_softmax.onnx_b1_gpu0_fp16.engine")
    model = TrtModel(trt_engine_path)
    shape = model.engine.get_binding_shape(0)

    image_path = os.path.join(".", "smoke_0001.jpg")
    image_bgr = cv2.imread(image_path, cv2.IMREAD_COLOR)
    image_bgr = cv2.resize(image_bgr, (512, 512), interpolation=cv2.INTER_LINEAR)
    
    mean = [146.24873628, 129.14695714, 102.30211553]
    std = [45.51520174, 45.51520174, 45.51520174]

    print ("image bgr")
    print(image_bgr)
    # Apply transformations to the image
    input_tensor = (image_bgr-mean)/std
    input_tensor = input_tensor[np.newaxis, ...]

    #data = np.random.randint(0,512,(batch_size,*shape[1:]))/512
    result = model(input_tensor, batch_size)
    print("numpy type")
    print(result[0].dtype)
    print(result[0].reshape(2, 512, 512))
    

    output = result[0].reshape(2, 512, 512)

    # Convert output to a segmentation mask
    output_predictions = output.argmax(0)

    print(output_predictions.min(), output_predictions.max())
    
    
    # Plot the original image and segmentation mask
    plt.figure(figsize=(10, 5))

    # Display original image
    plt.subplot(1, 2, 1)
    plt.imshow(cv2.cvtColor(image_bgr, cv2.COLOR_BGR2RGB) )
    plt.title("Original Image")
    plt.axis('off')

    # Display segmentation mask
    plt.subplot(1, 2, 2)
    #plt.imshow(output_predictions.cpu().numpy(), cmap='jet', alpha=0.5)
    plt.title("Segmentation Mask")
    plt.axis('off')

    plt.savefig("model_test_rasterization.pdf", dpi=150)
    plt.show()