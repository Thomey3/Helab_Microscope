Image Processing Pipeline
=========================

The image processing pipeline handles data transformation from raw detector signals to displayable images.

Pipeline Architecture
---------------------

.. code-block:: text

   Raw Data (PMT signals)
          │
          ▼
   ┌─────────────────┐
   │  Descan Stage   │  Un-scan the data
   └────────┬────────┘
            │
            ▼
   ┌─────────────────┐
   │ Intensity Map   │  Convert to intensity values
   └────────┬────────┘
            │
            ▼
   ┌─────────────────┐
   │  GPU Process    │  CUDA reconstruction (optional)
   └────────┬────────┘
            │
            ▼
   Final Image

ImagePipeline Class
-------------------

.. doxygenclass:: helab::ImagePipeline
   :members:
   :protected-members:
   :undoc-members:

Processing Stage Interface
--------------------------

.. doxygenclass:: helab::IProcessingStage
   :members:
   :undoc-members:

Built-in Processing Stages
--------------------------

DescanStage
~~~~~~~~~~~

.. doxygenclass:: helab::DescanStage
   :members:
   :undoc-members:

The descan stage reverses the scanning pattern to reconstruct the image:

.. code-block:: text

   Scanned Data:  1 2 3 4 5 6 7 8 9 10 ...
   Scan Pattern:  → → → → → → → → → →  (forward line)
                  ← ← ← ← ← ← ← ← ← ←  (backward line)

   Descanned:     Reconstruct 2D image from scan sequence

IntensityMapStage
~~~~~~~~~~~~~~~~~

.. doxygenclass:: helab::IntensityMapStage
   :members:
   :undoc-members:

Converts raw ADC values to image intensity:

.. code-block:: cpp

   // 14-bit ADC to 8-bit image
   uint8_t intensity = (adc_value >> 6) & 0xFF;

   // With lookup table for gamma correction
   uint8_t intensity = gammaLUT[adc_value];

GPU Acceleration
----------------

When CUDA is enabled, the pipeline uses GPU for processing:

.. code-block:: cpp

   pipeline.enableGpuAcceleration(true);

GPU Pipeline
~~~~~~~~~~~~

.. code-block:: text

   Host Memory ──cudaMemcpy──→ Device Memory
                                   │
                                   ▼
                            ┌─────────────┐
                            │ CUDA Kernel │
                            └─────────────┘
                                   │
                                   ▼
   Host Memory ←──cudaMemcpy── Device Memory

CUDA Kernel Operations
~~~~~~~~~~~~~~~~~~~~~~

1. **Descan**: Reorder pixels based on scan pattern
2. **Intensity Mapping**: Apply lookup table in parallel
3. **Averaging**: Multi-frame averaging for noise reduction
4. **Background Subtraction**: Remove background signal

Custom Processing Stages
------------------------

To create a custom processing stage:

.. code-block:: cpp

   class MyCustomStage : public IProcessingStage {
   public:
       void process(void* inputData, void* outputData, size_t size) override {
           // Custom processing logic
           auto* input = static_cast<uint16_t*>(inputData);
           auto* output = static_cast<uint8_t*>(outputData);

           for (size_t i = 0; i < size; ++i) {
               output[i] = transform(input[i]);
           }
       }

       std::string name() const override {
           return "MyCustomStage";
       }

   private:
       uint8_t transform(uint16_t value) {
           // Your transformation
           return static_cast<uint8_t>(value >> 8);
       }
   };

Adding to Pipeline
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   auto pipeline = std::make_unique<ImagePipeline>();

   // Add built-in stages
   pipeline->addStage(std::make_unique<DescanStage>());
   pipeline->addStage(std::make_unique<IntensityMapStage>());

   // Add custom stage
   pipeline->addStage(std::make_unique<MyCustomStage>());

Buffer Management
-----------------

BufferPool
~~~~~~~~~~

.. doxygenclass:: helab::BufferPool
   :members:
   :undoc-members:

The buffer pool manages memory for efficient image processing:

.. code-block:: cpp

   BufferPool pool(10, 512 * 512 * sizeof(uint16_t));  // 10 buffers

   auto buffer = pool.acquire();
   // ... use buffer ...
   pool.release(buffer);

Memory Layout
~~~~~~~~~~~~~

.. code-block:: text

   ┌─────────────────────────────────────────┐
   │              Ring Buffer                 │
   ├─────────┬─────────┬─────────┬───────────┤
   │ Buffer 0│ Buffer 1│ Buffer 2│    ...    │
   ├─────────┼─────────┼─────────┼───────────┤
   │ 512x512 │ 512x512 │ 512x512 │    ...    │
   │ x 2bytes│ x 2bytes│ x 2bytes│           │
   └─────────┴─────────┴─────────┴───────────┘

Thread Safety
-------------

The pipeline is designed for multi-threaded operation:

- **Acquisition thread**: Writes to input buffer
- **Processing thread**: Reads input, writes output
- **Display thread**: Reads output buffer

Synchronization
~~~~~~~~~~~~~~~

.. code-block:: cpp

   // Thread-safe queue between stages
   ThreadSafeQueue<ImageBuffer> queue;

   // Acquisition thread
   queue.push(rawBuffer);

   // Processing thread
   auto buffer = queue.pop();
   pipeline->process(buffer.data, buffer.size, output);
