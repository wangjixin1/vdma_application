#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/mman.h>

#define mm2s_ctrl_reg 0x00
#define mm2s_status_reg 0x04
#define mm2s_source_addr_reg 0x18
#define mm2s_length_reg 0x28

#define s2mm_ctrl_reg 0x30
#define s2mm_status_reg 0x34
#define s2mm_dest_addr_reg 0x48
#define s2mm_length_reg 0x58

#define source_address 0x10000000
#define destination_address 0x20000000
#define transfer_length 512

unsigned int dma_write(unsigned int* dma_dma_reg_addr, int reg_offset, unsigned int value);
unsigned int dma_read(unsigned int* dma_dma_reg_addr, int reg_offset);
void read_data(void* adress, int byte_length);
void dma_status_read(unsigned int* dma_dma_reg_addr);
int dma_s2mm_idle(unsigned int* dma_dma_reg_addr);
int dma_mm2s_idle(unsigned int* dma_dma_reg_addr);
void load_data(void* adress, int byte_length);
int main() {
    int dh = open("/dev/mem", O_RDWR | O_SYNC); 
    unsigned int* dma_reg_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0000000); // AXI Lite address
    unsigned int* dma_source_addr  = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, dh, source_address); // MM2S SOURCE address
    unsigned int* dma_distination_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, dh, destination_address); // S2MM DESTINATION address

	printf("running AXI DMA...\r\n Source address : 0x%x  \t\t Destination address: 0x%x\r\n",source_address,destination_address);
	load_data(dma_source_addr,transfer_length)
    //reset the channels
    dma_write(dma_reg_addr, s2mm_ctrl_reg, 4);
    dma_write(dma_reg_addr, mm2s_ctrl_reg, 4);
   
    //halt the channels
    dma_write(dma_reg_addr, s2mm_ctrl_reg, 0);
    dma_write(dma_reg_addr, mm2s_ctrl_reg, 0);
    	

	// set source and destination addresses
	dma_write(dma_reg_addr, mm2s_source_addr_reg, source_address); // Write source address
    dma_write(dma_reg_addr, s2mm_dest_addr_reg, destination_address); // Write destination address
		
	

    //Mask interrupts
    dma_write(dma_reg_addr, s2mm_ctrl_reg, 0xf001);
    dma_write(dma_reg_addr, mm2s_ctrl_reg, 0xf001);
    

    // set transfer length
    dma_write(dma_reg_addr, s2mm_length_reg, transfer_length);
    dma_write(dma_reg_addr, mm2s_length_reg, transfer_length); 
	printf("started dma transfer\r\n");
    dma_status_read(dma_reg_addr);


    // wait for idle bit
    dma_mm2s_idle(dma_reg_addr);
    dma_s2mm_idle(dma_reg_addr); 
	printf("dma transfer completed\r\n");
    dma_status_read(dma_reg_addr);

    printf("data at destination address 0x%x: ",dma_distination_addr); 
	read_data(dma_distination_addr, transfer_length);
	
	printf("\r\n\r\n");
}

unsigned int dma_write(unsigned int* dma_dma_reg_addr, int reg_offset, unsigned int value) 
{
    dma_dma_reg_addr[reg_offset>>2] = value;
}

unsigned int dma_read(unsigned int* dma_dma_reg_addr, int reg_offset)
{
    return dma_dma_reg_addr[reg_offset>>2];
}

int dma_mm2s_idle(unsigned int* dma_dma_reg_addr) 
{
    unsigned int mm2s_status =  dma_read(dma_dma_reg_addr, mm2s_status_reg);
    while(!(mm2s_status & 0x02) )
	{
        mm2s_status =  dma_read(dma_dma_reg_addr, mm2s_status_reg);
    }
}

int dma_s2mm_idle(unsigned int* dma_dma_reg_addr) 
{
    unsigned int s2mm_status = dma_read(dma_dma_reg_addr, s2mm_status_reg);
    while(!(s2mm_status & 0x02)){
        s2mm_status = dma_read(dma_dma_reg_addr, s2mm_status_reg);
    }
}

void dma_status_read(unsigned int* dma_dma_reg_addr) 
{
    unsigned int status;
	status = dma_read(dma_dma_reg_addr, s2mm_status_reg);
    printf("S2MM status reg:0x%x\n", status, s2mm_status_reg);

    status = dma_read(dma_dma_reg_addr, mm2s_status_reg);
	printf("MM2S status reg:0x%x\n", status, mm2s_status_reg);
    
}

void read_data(void* adress, int byte_length) 
{
    int *addr = adress;
    int reg_offset;
    for (reg_offset = 0; reg_offset < byte_length/4; reg_offset+=4) {
        printf("%x\t", addr[reg_offset]);
    }
    printf("\n");
}
void load_data(void* adress, int byte_length) 
{
    int *addr = adress;
    int reg_offset;
    for (reg_offset = 0; reg_offset < byte_length/4; reg_offset+=4) {
		addr[reg_offset]=0x12345678+reg_offset;
        printf("%x\t", addr[reg_offset]); 
    }
    printf("\n");
}