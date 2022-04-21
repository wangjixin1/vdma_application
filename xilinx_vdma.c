#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/mman.h>

/*********************************************************************
 * xilinx_vdma
 * @S2MM: vdma s2mm register define
 ********************************************************************/
#define S2MM_VDMACR_REG         0x30 //S2MM VDMA Control Register
#define S2MM_VDMASR_REG         0x34 //S2MM VDMA Status Register
/**
* Set S2MM_Start_Address 1 (ACh) through S2MM_Start_Address 3 (B4h) 
* to their required locations. These locations can be static (based
* on maximum frame size) or dynamic (based on actual frame size). 
*/
#define S2MM_START_ADDRESS_1    0xAC //Write a valid video frame buffer start address to the channel 
#define S2MM_START_ADDRESS_2    0xB0
#define S2MM_START_ADDRESS_3    0xB4
#define S2MM_FRMDLY_STRIDE      0xA8 //Write a valid Frame Delay (valid only for Genlock Slave) and Stride to the channel FRMDLY_STRIDE register
#define S2MM_HORIZONTAL_SIZE    0xA4 //Write a valid Horizontal Size to the channel HSIZE register
#define S2MM_VERTICAL_SIZE      0xA0 //Write a valid Vertical Size to the channel VSIZE register

#define DESTINATION_ADDRESS_1   0x50000000
#define DESTINATION_ADDRESS_2   0x60000000
#define DESTINATION_ADDRESS_3   0x70000000

/*********************************************************************
 * xilinx_vdma
 * @MM2S: vdma mm2s register define
 ********************************************************************/
#define MM2S_VDMACR_REG         0x00 //MM2S VDMA Control Register
#define MM2S_VDMASR_REG         0x04 //MM2S VDMA Status Register

#define MM2S_START_ADDRESS      0x5C
#define MM2S_FRMDLY_STRIDE      0x58
#define MM2S_HORIZONTAL_SIZE    0x54
#define MM2S_VERTICAL_SIZE      0x50

#define SOURCE_ADDRESS          0x40000000
#define MMAP_SIZE               0x10000

#define AXILITE_BASEADDR        0xA0030000
#define FRAME_HORIZONTAL_LEN    0x2D00   // 3840 pixels, each pixel 3 bytes
#define FRAME_VERTICAL_LEN      0x870    // 2160 pixels 

#define PL_PS_COM_REG           0xA0050000
#define PL_PS_COM_VALUE         0x01020304

#define transfer_length         3840*2160*3


unsigned int vdma_write(unsigned int* dma_vdma_reg_baseaddr, int reg_offset, unsigned int value);
unsigned int vdma_read(unsigned int* dma_vdma_reg_baseaddr, int reg_offset);
void read_data(void* adress, int byte_length);
void vdma_status_read(unsigned int* dma_vdma_reg_baseaddr);
int vdma_s2mm_ptr(unsigned int* dma_vdma_reg_baseaddr);

//void load_data(void* adress, int byte_length);

unsigned int vdma_write(unsigned int* dma_vdma_reg_baseaddr, int reg_offset, unsigned int value) 
{
    dma_vdma_reg_baseaddr[reg_offset >> 2] = value;
}

unsigned int vdma_read(unsigned int* dma_vdma_reg_baseaddr, int reg_offset)
{
    return dma_vdma_reg_baseaddr[reg_offset >> 2];
}

int vdma_s2mm_ptr(unsigned int* dma_vdma_reg_baseaddr) 
{
    unsigned int s2mm_status = vdma_read(dma_vdma_reg_baseaddr, 0x28);
    printf ("PARK_PTR_REG value is %#x \n", s2mm_status);
    while( !(s2mm_status & 0x02000000) )
        s2mm_status = vdma_read(dma_vdma_reg_baseaddr, 0x28);
}

void vdma_status_read(unsigned int* dma_vdma_reg_baseaddr) 
{
    unsigned int status;
	status = vdma_read(dma_vdma_reg_baseaddr, S2MM_VDMASR_REG);
    printf("S2MM status reg: 0x%x\n", status);

    //status = vdma_read(dma_vdma_reg_baseaddr, MM2S_VDMASR_REG);
	//printf("MM2S status reg: 0x%x\n", status);
}

void read_data(void* adress, int byte_length) 
{
    int *addr = adress;
    int reg_offset;
    for (reg_offset = 0; reg_offset < byte_length / 4; reg_offset += 4) {
        printf("%x\t", addr[reg_offset]);
    }
    printf("\n");
}

void load_data(void* adress, int byte_length) 
{
    int *addr = adress;
    int reg_offset;
    for (reg_offset = 0; reg_offset < byte_length / 4; reg_offset += 4) {
		addr[reg_offset] = 0x12345678 + reg_offset;
        printf("%x\t", addr[reg_offset]); 
    }
    printf("\n");
}

int main() 
{
    int fd;
    
    unsigned int* vdma_reg_baseaddr = NULL;
    unsigned int* pl_ps_com_reg = NULL;
    unsigned int* dma_source_addr = NULL;
    unsigned int* dma_dist_addr_1 = NULL;
    unsigned int* dma_dist_addr_2 = NULL;
    unsigned int* dma_dist_addr_3 = NULL;

    fd = open("/dev/mem", O_RDWR | O_SYNC); 
    if (fd < 0){
        printf("open /dev/mem  fd failed \n");
        return -1;
    }

    vdma_reg_baseaddr = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, AXILITE_BASEADDR); // AXI Lite base address
    if (NULL == vdma_reg_baseaddr){
        printf("vdma_reg_baseaddr is NULL \n");
        return -1;
    }
    else {
        printf ("vdma_reg_baseaddr map successfull, the address is %x \n", (unsigned int*)vdma_reg_baseaddr);
    }

    pl_ps_com_reg = mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, PL_PS_COM_REG);
    if (NULL == pl_ps_com_reg){
        printf("pl_ps_com_reg is NULL \n");
        return -1;
    }
    else {
        printf ("pl_ps_com_reg map successfull, the address is %x \n", (unsigned int*)pl_ps_com_reg);
    }

    dma_dist_addr_1 = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DESTINATION_ADDRESS_1); // S2MM DESTINATION address
    if (NULL == dma_dist_addr_1){
        printf("dma_dist_addr_1 is NULL \n");
        return -1;
    }
    else {
        printf ("dma_dist_addr_1 map successfull, the address is %x \n", (unsigned int*)dma_dist_addr_1);
    }

    dma_dist_addr_2 = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DESTINATION_ADDRESS_2); // S2MM DESTINATION address
    if (NULL == dma_dist_addr_2){
        printf("dma_dist_addr_2 is NULL \n");
        return -1;
    }
    else {
        printf ("dma_dist_addr_2 map successfull, the address is %x \n", (unsigned int*)dma_dist_addr_2);
    }

    dma_dist_addr_3 = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, fd, DESTINATION_ADDRESS_3); // S2MM DESTINATION address
    if (NULL == dma_dist_addr_3){
        printf("dma_dist_addr_3 is NULL \n");
        return -1;
    }
    else {
        printf ("dma_dist_addr_3 map successfull, the address is %x \n", (unsigned int*)dma_dist_addr_3);
    }

    /* vdma S2MM channel configration */
    vdma_write(vdma_reg_baseaddr, S2MM_VDMACR_REG, 0x04);//S2MM_VDMACR reset the channels | set bit2 = 1
    vdma_write(vdma_reg_baseaddr, S2MM_VDMACR_REG, 0x00);  
    vdma_write(vdma_reg_baseaddr, S2MM_VDMACR_REG, 0x8B);//S2MM_VDMACR Mask interrupts | set bit0 bit1 bit3 bit7
    //S2MM_DA set destination addresses
    vdma_write(vdma_reg_baseaddr, S2MM_START_ADDRESS_1, DESTINATION_ADDRESS_1); // Write destination address_1
    vdma_write(vdma_reg_baseaddr, S2MM_START_ADDRESS_2, DESTINATION_ADDRESS_2); // Write destination address_2
    vdma_write(vdma_reg_baseaddr, S2MM_START_ADDRESS_3, DESTINATION_ADDRESS_3); // Write destination address_3
    //S2MM_LENGTH set transfer length bit0-bit25
    vdma_write(vdma_reg_baseaddr, S2MM_FRMDLY_STRIDE, FRAME_HORIZONTAL_LEN);
    vdma_write(vdma_reg_baseaddr, S2MM_HORIZONTAL_SIZE, FRAME_HORIZONTAL_LEN);
    vdma_write(vdma_reg_baseaddr, S2MM_VERTICAL_SIZE, FRAME_VERTICAL_LEN);

    vdma_write(pl_ps_com_reg, 0x00, PL_PS_COM_VALUE);

    //read PARK_PTR_REG index point 2
    vdma_s2mm_ptr(vdma_reg_baseaddr); 
	printf("vdma S2MM transfer PARK_PTR_REG index is completed 2 \n");
    vdma_status_read(vdma_reg_baseaddr);

    read_data(dma_dist_addr_1, transfer_length);
    
    printf("data at destination address 0x%x: ", (unsigned int*)dma_dist_addr_1); 

    close (fd);
    munmap (vdma_reg_baseaddr, MMAP_SIZE);
    munmap (pl_ps_com_reg, MMAP_SIZE);
    munmap (dma_dist_addr_1, 65535);
    munmap (dma_dist_addr_2, 65535);
    munmap (dma_dist_addr_3, 65535);
     
    return 1;
}

   
#if 0

    dma_source_addr = mmap(NULL, 65535, PROT_READ | PROT_WRITE, MAP_SHARED, fd, SOURCE_ADDRESS);   // MM2S SOURCE address
	printf("running AXI VDMA...\r\n Source address : 0x%lx  \t\t Destination address: 0x%lx\r\n", SOURCE_ADDRESS, DESTINATION_ADDRESS_1);
    
    //writes data to the source address 
	//load_data(dma_source_addr, transfer_length);


    //MM2S_VDMACR reset the channels | set bit2 = 1
    vdma_write(vdma_reg_baseaddr, MM2S_VDMACR_REG, 4);
    //halt the channels
    vdma_write(vdma_reg_baseaddr, MM2S_VDMACR_REG, 0);
    //MM2S_DMACR Mask interrupts |
    vdma_write(vdma_reg_baseaddr, MM2S_VDMACR_REG, 0x8B);    	
	//MM2S_SA set source addresses
	vdma_write(vdma_reg_baseaddr, MM2S_START_ADDRESS, SOURCE_ADDRESS); // Write source address
    //MM2S_LENGTH set transfer length bit0-bit25
    vdma_write(vdma_reg_baseaddr, MM2S_FRMDLY_STRIDE, FRAME_HORIZONTAL_LEN);
    vdma_write(vdma_reg_baseaddr, MM2S_HORIZONTAL_SIZE, FRAME_HORIZONTAL_LEN);
    vdma_write(vdma_reg_baseaddr, MM2S_VERTICAL_SIZE, FRAME_VERTICAL_LEN);
	printf("started dma transfer\r\n");

    dma_status_read(vdma_reg_baseaddr);



    dma_status_read(vdma_reg_baseaddr);

    printf("data at destination address 0x%x: ", dma_dist_addr_1); 
	
    read_data(dma_dist_addr_1, transfer_length);
	
	printf("\r\n\r\n");
#endif

