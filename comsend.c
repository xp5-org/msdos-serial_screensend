#include <stdio.h>
#include <dos.h>

#define VIDEO_MEMORY 0xB800  // mem starting location
#define ROWS 25
#define COLUMNS 80
#define COM1 0x3F8
#define BAUD_RATE 300 


void sendtoserial(unsigned char byte) {
    while ((inportb(COM1 + 5) & 0x20) == 0);  // need to wait until tx buffer is empty
    outportb(COM1, byte);  // send one byte
}

int main() {
    unsigned char far *video;
    int row, col;
    unsigned long divisor;
    unsigned long divisor_lsb, divisor_msb;
    FILE *output;
    video = (unsigned char far *)MK_FP(VIDEO_MEMORY, 0);  // pointer for video mem

    // open output.txt with write access
    output = fopen("output.txt", "w");
    if (output == NULL) {
        printf("could not open output.txt in write mode\n");
        return 1;
    }

    // Calculate the divisor for the given baud rate
    divisor = (1843200 / (BAUD_RATE * 16));  // 1.8432 MHz UART clock
    divisor_lsb = (unsigned long)(divisor & 0xFF);  // LSB
    divisor_msb = (unsigned long)((divisor >> 8) & 0xFF);  // MSB

    // setup serial port
    outportb(COM1 + 1, 0x00);  // interrupts off
    outportb(COM1 + 3, 0x80);  // divisor latch
    outportb(COM1 + 0, divisor_lsb);  // LSB
    outportb(COM1 + 1, divisor_msb);  // MSB
    outportb(COM1 + 3, 0x03);  // set 8N1
    outportb(COM1 + 2, 0xC7);  // enable & clear fifo
    outportb(COM1 + 4, 0x0B);  // enable tx

    // loop through the memory rows/columns and pass around the video pointer
    for (row = 0; row < ROWS; row++) {
        for (col = 0; col < COLUMNS; col++) {
            unsigned char character = *video;  // get 1 byte from mem pointer
            video += 2;  // skip attributes byte (color?)
            fputc(character, output);  // file - output.txt
            sendtoserial(character); // serial - 1 byte at a time
        }
        
        // for the output.txt file
        fputc('\n', output);  // add a newline

        // for serial only, need both
        sendtoserial(0x0D);  // Carriage Return
        sendtoserial(0x0A);  // Line Feed
    }

    fclose(output);  // close file handle
    printf("done. saved to output.txt & sent to COM1\n");

    return 0;
}
