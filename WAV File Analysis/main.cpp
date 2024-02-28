//
//  main.cpp
//  as2
//
//  Created by Ares Mutemet on 29/01/2023.
//

#include <iostream>
#include <vector>
#include <cmath>

using namespace std;

struct WAV_HEADER {
    uint8_t RIFF[4]; // RIFF Header Magic header
    uint32_t ChunkSize; // RIFF Chunk Size
    uint8_t WAVE[4]; // WAVE Header
    uint8_t fmt[4]; // FMT header
    uint32_t Subchunk1Size; // Size of fmt chunk
    uint16_t AudioFormat; // Audio format 1=PCM,6=mulaw,7=alaw
    uint16_t NumOfChan; // Number of Channels 1=mono, 2 =stereo
    uint32_t SamplesPerSec; // Sampling Frequency in Hz
    uint32_t bytesPerSec; // bytes per second
    uint16_t blockAlign; // 2=16-bit mono, 4=16-bit stereo
    uint16_t bitsPerSample; // Number of bits per sample
    uint8_t Subchunk2ID[4]; // "data" string
    uint32_t Subchunk2Size; // Sampled data length
} wav_hdr;

int main() {

    // Declare a WAV_HEADER struct variable to hold the header information of the input wav file
    WAV_HEADER wavHeader;
    // Declare a variable to hold the size of the header and set it to the size of the WAV_HEADER struct
    int headerSize = sizeof(wav_hdr);
    // Open the input wav file in read mode and store the file pointer in wavFile
    FILE* wavFile = fopen("input.wav","r");
    // Read the header information from the input wav file and store it in the wavHeader variable
    fread(&wavHeader,1,headerSize, wavFile);
    
    // Declare a variable to hold the length of the audio data in the input wav file and set it to the value of the Subchunk2Size field in the header
    int length = wavHeader.Subchunk2Size;
    // Allocate memory to store the audio data using the length variable
    int8_t *data = new int8_t[length];
    // Seek to the beginning of the audio data in the input wav file
    fseek(wavFile, headerSize, SEEK_SET);

    // Read the audio data from the input wav file and store it in the data variable
    fread(data, sizeof(data[0]), length / (sizeof(data[0])), wavFile);

    // Declare a vector to hold the new signal
    vector<double> NewSignal;

    
    // loop through the data array and convert the 16-bit integer data to a double
    for (int i=0; i < length; i=i+2)
    {
    // combine two 8-bit integers into one 16-bit integer
    int c = ((data[i] & 0xff) | (data[i+1] << 8));
    double t;
        // check if we are still within the bounds of the data array
        if (i<=length) {
            // divide by 32767.0 to convert the 16-bit integer value to a double between -1 and 1
            t = c / 32767.0;
        } else {
            // if we are out of bounds, set the value to 0.0
            t=0.0;
        }
        // add the value to the NewSignal vector
        NewSignal.push_back(t);
    }

    // The original sample rate of the audio file is stored in the wavHeader struct
    int originalSampleRate = wavHeader.SamplesPerSec;
    // The desired sample rate that we want to convert the audio file to
    int desiredSampleRate = 48000;
    // Calculate the ratio between the original and desired sample rates
    double ratio = (double)desiredSampleRate / (double)originalSampleRate;
    // Calculate the new length of the audio file based on the ratio
    int newLength = (int)(NewSignal.size() * ratio);
    // Create a vector to store the interpolated data
    vector<double> interpolatedData(newLength);
    // Use linear interpolation to fill in the interpolatedData vector
    for (int i = 0; i < newLength; i++) {
        // Calculate the position in the original data for the current sample
        double position = i / ratio;
        // Get the index of the sample to the left and right of the current position
        int leftSampleIndex = (int)floor(position);
        int rightSampleIndex = (int)ceil(position);
        // Calculate the interpolation coefficient
        double alpha = position - leftSampleIndex;
        // If we've reached the end of the original data, just use the last sample
        if (rightSampleIndex >= NewSignal.size()) {
                interpolatedData[i] = NewSignal[NewSignal.size() - 1];
        }
        // Otherwise, use linear interpolation to calculate the new sample value
        else {
            interpolatedData[i] = (1.0 - alpha) * NewSignal[leftSampleIndex] + alpha * NewSignal[rightSampleIndex];
        }
    }

    // Update the header with the new length and sample rate
    wavHeader.ChunkSize = 36 + (newLength*2);
    wavHeader.Subchunk2Size = newLength * 2; // update the size of the data section
    wavHeader.SamplesPerSec = desiredSampleRate; // update the sample rate
    wavHeader.bytesPerSec = desiredSampleRate * 2; // update the bytes per second
    wavHeader.blockAlign = 2; // update the block align
    
    // open a new wav file for writing
    FILE* fptr2;
    fptr2 = fopen("Upsample_Audio.wav", "wb");
    // write the updated header to the new file
    fwrite(&wavHeader, sizeof(wavHeader), 1, fptr2);
    // write the new data to the new file.
    for (int i = 0; i < newLength; i++) {
            int value = (int)(32767 * interpolatedData[i]);
            int size = 2;
            for (; size; --size, value >>= 8) {
                fwrite(&value, 1, 1, fptr2);
            }
    }
    fclose(fptr2); // close the file
    // done with upsampling and saving the new file
    
    int N = length / (wavHeader.bitsPerSample / 8);
    int start_mute = N/2 + N/100; // Start muting from the middle of the audio file, plus 1% of the total length
    int end_mute = N/2 + N/30; // End muting at the middle of the audio file, plus 3.3% of the total length

    // This loop sets the amplitude of the audio samples in the specified range to zero, effectively muting the audio
    for (int i = start_mute; i < end_mute; i++) {
            interpolatedData[i] = 0;
    }

    // The following code writes the new, muted audio data to a new file
    FILE* fptr3;
    fptr3 = fopen("Muted_Audio.wav", "wb"); // Open a new file for writing in binary mode
    fwrite(&wavHeader, sizeof(wavHeader), 1, fptr3); // Write the wav header to the new file
    // This loop writes the audio samples to the new file
    for (int i = 0; i < newLength; i++) {
        int value = (int)(32767 * interpolatedData[i]); // Convert the sample value to a 16-bit integer
        int size = 2;
        for (; size; --size, value >>= 8) { // Write the sample value to the file in little-endian order
                fwrite(&value, 1, 1, fptr3);
        }
    }
    fclose(fptr3); // Close the file
    
    return 0;
}
