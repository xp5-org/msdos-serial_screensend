#include <stdio.h>
#include <dos.h>

#define VIDEO_MEMORY 0xB800  // mem starting location
#define ROWS 25
#define COLUMNS 80
#define COM1 0x3F8
#define BAUD_RATE 1200 // set to a valid baudrate


unsigned char far *detect_video_memory() {
    union REGS regs;
    unsigned char mode;
    regs.h.ah = 0x0F;  // get current video mode
    int86(0x10, &regs, &regs);
    mode = regs.h.al; 

    if (mode == 0x07) {  // monochrome
        return (unsigned char far *)MK_FP(0xB000, 0);
    } else {  // color mode for CGA EGA VGA
        return (unsigned char far *)MK_FP(0xB800, 0);
    }
}


void sendtoserial(unsigned char byte) {
    while ((inportb(COM1 + 5) & 0x20) == 0);  // wait until tx buffer is empty
    outportb(COM1, byte);  // send one byte
}

int main() {
    unsigned char far *video;
    int row, col;
    unsigned int divisor;
    unsigned int divisor_lsb, divisor_msb;
    FILE *output;
    video = detect_video_memory();  // get & set pointer for video mem


    // open output.txt with write access
    output = fopen("output.txt", "w");
    if (output == NULL) {
        printf("could not open output.txt in write mode\n");
        return 1;
    }

    // set divisor for the baud rate
    divisor = (1843200 / (BAUD_RATE * 16));  // 1.8432 MHz UART clock
    divisor_lsb = (unsigned int)(divisor & 0xFF);  // LSB
    divisor_msb = (unsigned int)((divisor >> 8) & 0xFF);  // MSB

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
        sendtoserial(0x0D);  // CR
        sendtoserial(0x0A);  // LF
    }

    fclose(output);  // close file handle
    printf("done. saved to output.txt & sent to COM1\n");

    return 0;
}
