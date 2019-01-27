#pragma once

#include "PixelWriterAsync.h"

class PixelBuffer
{
public:
    PixelBuffer(PixelWriterAsync* pixelWriter, const uint8_t pixelSize, const uint8_t pixelCount, const uint8_t pins[], const uint8_t pinCount) 
    : _pixelWriter(pixelWriter), PINS(pins), PIN_COUNT(pinCount), MAX_BUFFER_SIZE(pixelSize * pixelCount)
    {
        // Default the buffer size to the max, so we don't have to multiply twice
        bufferSize = MAX_BUFFER_SIZE;
           
        // Allocate the memory and zero it out.
        buffer = (uint8_t*)malloc(MAX_BUFFER_SIZE);
        Clear();
    };

    ~PixelBuffer()
    {
      // Release the memory
      free(buffer);
    };

    // Set buffer from a repeating array
    void SetRepeat(const uint8_t* data, const uint8_t dataLength)
    {
        uint16_t offset = 0;

        // Do this all up until a full set of data cannot be copied.
        while(offset + dataLength < bufferSize){

            // Copy the data to the buffer 
            memcpy(buffer + offset, data, dataLength);
  
            // Increment the current buffer pointer
            offset += dataLength;
        }
    }

    // Set buffer from an array
    void SetBuffer(const uint8_t* data, const uint16_t dataLength)
    {
        // Save the latest buffer size, limiting it to the max on the top end
        bufferSize = _min(MAX_BUFFER_SIZE, dataLength);

        // Copy the data to the buffer
        memcpy(buffer, data, bufferSize);
    }

    // Clear the pixels, requires a call to show to complete
    void Clear()
    {
        // Clear the entire buffer.
        memset(buffer, 0x00, MAX_BUFFER_SIZE);    
    }

    // using the pixel writer sends the pixel string out
    void Show()
    {
        _pixelWriter->Update(PINS, PIN_COUNT, buffer, bufferSize);   
    }

private:
  const uint8_t* PINS;        // Array of output pins associated with this buffer
  const uint8_t PIN_COUNT;    // The count of pins associated with this buffer
  const uint16_t MAX_BUFFER_SIZE;   // Maximum possible size of 'buffer' below

  uint16_t bufferSize;    // Size of the buffer
  uint8_t* buffer;        // The actual data buffer
  
  PixelWriterAsync* _pixelWriter;
  
};
