/*-------------------------------------------------------------------------
NeoPixel library helper functions for Esp8266 UART hardware
Written by Michael C. Miller.
I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)
-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.
NeoPixelBus is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.
NeoPixelBus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.
You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/

#pragma once

#ifdef ARDUINO_ARCH_ESP8266
#include <Arduino.h>

// PixelWriterAsync handles all transmission asynchronously using interrupts
//
// This UART controller uses two buffers that are swapped in every call to
// NeoPixelBus.Show(). One buffer contains the data that is being sent
// asynchronosly and another buffer contains the data that will be send
// in the next call to NeoPixelBus.Show().
//
// Therefore, the result of NeoPixelBus.Pixels() is invalidated after
// every call to NeoPixelBus.Show() and must not be cached.
class PixelWriterAsync
{ 
public:
    PixelWriterAsync(const uint16_t maxPixelCount, const uint8_t pixelSize, const uint8_t pins[], const uint8_t pinCount);

    ~PixelWriterAsync();

    bool IsReadyToUpdate() const
    {
        uint32_t delta = micros() - this->_startTime;

        return delta >= this->_pixelTime;
    }

    void Initialize()
    {
        // Setup the pins
        uint8_t pinIndex;
        for(pinIndex = 0; pinIndex < this->PIN_COUNT; pinIndex++)
        {
          pinMode(this->PINS[pinIndex], OUTPUT);
          digitalWrite(this->PINS[pinIndex], HIGH);     
        }
      
        this->InitializeUart(UartBaud);

        // Inverting logic levels can generate a phantom bit in the led strip bus
        // We need to delay 50+ microseconds the output stream to force a data
        // latch and discard this bit. Otherwise, that bit would be prepended to
        // the first frame corrupting it.
        this->_startTime = micros();
    }

    void Update(const uint8_t pins[], const uint8_t pinCount, const uint8_t* data, const uint16_t dataLength)
    {
        // Data latch = 50+ microsecond pause in the output stream.  Rather than
        // put a delay at the end of the function, the ending time is noted and
        // the function will simply hold off (if needed) on issuing the
        // subsequent round of data until the latch time has elapsed.  This
        // allows the mainline code to start generating the next frame of data
        // rather than stalling for the latch.
        // This is compounded when running multiple outputs...
        // For multiple outputs, the minimum we wait is for the previous message to complete.
        // We also wait for the completion time of any pins we will be writing to for them to complete
        // plus their data latch time.  This allow the fastest transition from sending to one output
        // to the next, while maintaining the latch if we are rewriting to the same output in a hurry.

        // Check if we need to wait for previous update to complete
        while (!this->IsReadyToUpdate())
        {
          yield();
        }

        // Disable all the pins
        uint8_t pinIndex;
        for(pinIndex = 0; pinIndex < PIN_COUNT; pinIndex++)
        {
          digitalWrite(PINS[pinIndex], HIGH);         
        }

        // Enable any pins
        for(pinIndex = 0; pinIndex < pinCount; pinIndex++)
        {
          digitalWrite(pins[pinIndex], LOW);        
        }

        // Run the update
        this->UpdateUart(data, dataLength);    
    }

private:

    // 400kbps
    //static const uint32_t ByteSendTimeUs = 20; // us it takes to send a single pixel element at 400khz speed
    //static const uint32_t UartBaud = 1600000; // 400mhz, 4 serial bytes per NeoByte

    // 800kbps
    static const uint32_t ByteSendTimeUs =  10; // us it takes to send a single pixel element at 800khz speed
    static const uint32_t UartBaud = 3200000; // 800mhz, 4 serial bytes per NeoByte
    static const uint8_t LatchTime = 0; //Latch time is not so important with multiple outputs running in parallel; // 50us, the time required for the neopixels to latch the sent data

    static void ICACHE_RAM_ATTR IntrHandler(void* param);  

    void InitializeUart(uint32_t uartBaud);

    void UpdateUart(const uint8_t* data, uint16_t dataLength);

    static const uint8_t* ICACHE_RAM_ATTR FillUartFifo(const uint8_t* pixels, const uint8_t* end);

    const uint8_t MAX_PIXEL_COUNT;
    const uint8_t* PINS;
    const uint8_t PIN_COUNT;

    uint16_t _bufferSize;   // Size of '_asyncBuffer' buffer below
    uint8_t* _asyncBuffer;  // Holds a copy of LED color values taken when UpdateUart began
    uint32_t _startTime;     // Microsecond count when last update started
    uint32_t _pixelTime;  // Time to send an entire array of pixels

};

#endif
