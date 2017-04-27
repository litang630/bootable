#ifndef BUILD_LK
#include <linux/string.h>
#else
#include <string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
	#include <platform/mt_pmic.h>
	#include <platform/mt_i2c.h>
    #include <platform/upmu_common.h>	
#elif defined(BUILD_UBOOT)
#else
	#include <mach/mt_gpio.h>
	#include <mach/mt_pm_ldo.h>
#include <mach/upmu_common.h>
#include <mach/pmic_mt6325_sw.h>
#include <mach/upmu_hw.h>   
#endif


const static unsigned char LCD_MODULE_ID = 0x94;//ID0->1;ID1->X
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID_T80P20										0x94

#define GPIO_LCM_RST	(GPIO146 | 0x80000000)
#define GPIO_LCD_V18_EN     (GPIO4 | 0x80000000)
#define GPIO_LCD_V33_EN     (GPIO3 | 0x80000000)


#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

#define REGFLAG_DELAY             								0xFC
#define REGFLAG_END_OF_TABLE      							0xFD   // END OF REGISTERS MARKER

#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

#define LCM_TABLE_V3


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

const static unsigned int BL_MIN_LEVEL =20;
static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V3(ppara, size, force_update)	        	lcm_util.dsi_set_cmdq_V3(ppara, size, force_update)
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)      

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128];
};

static void lcm_set_gpio_output(unsigned int GPIO, unsigned int output)
{
  if(GPIO == 0xFFFFFFFF)
  {
  #ifdef BUILD_LK
    printf("[LK/LCM] invalid gpio\n");	
  #else	
  	printk("[LK/LCM] invalid gpio\n");	
  #endif
    return;
  }

  mt_set_gpio_mode(GPIO, GPIO_MODE_00);
  mt_set_gpio_dir(GPIO, GPIO_DIR_OUT);
  mt_set_gpio_out(GPIO, (output>0)? GPIO_OUT_ONE: GPIO_OUT_ZERO);
}

static void lcd_reset(unsigned char enabled)
{
    if (enabled)
    {
        mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    }
    else
    {	
        mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
        mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);    	
    }
}

static void lcd_bl_en(unsigned char enabled)
{
}
static LCM_setting_table_V3 lcm_initialization_setting_V3[] = {

	{0x39, 0xB9, 	3,	{0xFF, 0x83, 0x94}},

	{0x39, 0xBA, 	16,	{0x13, 0x82, 0x00,0x16,
										 0xC5, 0x00, 0x10,0xFF, 
										 0x0F, 0x24, 0x03,0x21, 
										 0x24, 0x25, 0x20,0x08}},
	
	{0x39, 0xB1, 	17,	{0x01, 0x00, 0x04,
		 								 0xC7, 0x03, 0x12, 0xF1,
		 								 0x3a, 0x42, 0x29, 0x29,
		 								 0x57, 0x02, 0x00, 0xE6,
		 								 0xE2, 0xA6}},//36,3e
	
	{0x39, 0xB2, 	6,	{0x00, 0xC8, 0x0E,
		 								 0x30, 0x00, 0xE1}},		 								

	{0x39, 0xB4, 	31,	{0x80, 0x04, 0x32,
		 								 0x10, 0x08, 0x54, 0x15,
		 								 0x0F, 0x22, 0x10, 0x08,
		 								 0x47, 0x43, 0x44, 0x0A,
		 								 0x4B, 0x43, 0x44, 0x02,
		 								 0x55, 0x55, 0x02, 0x06,		 								 		 								 
		 								 0x44, 0x06, 0x5F, 0x0A,
		 								 0x6B, 0x70, 0x05, 0x08}},
	
	{0x15, 0xB6,	1,	{0x3D}},	

	{0x15, 0xCC,	1,	{0x09}},			
	{0x15, 0x51,	1,	{0xFF}},
	
	{0x39, 0xBF,	4,	{0x06, 0x02, 0x10, 0x04}},		

	{0x39, 0xC7,	4,	{0x00, 0x10, 0x00, 0x10}},		

	{0x39, 0xE0, 	42,	{0x00, 0x18, 0x1B,
		 								 0x31, 0x35, 0x3F, 0x28,
		 								 0x41, 0x06, 0x0C, 0x0E,
		 								 0x13, 0x15, 0x13, 0x14,
		 								 0x11, 0x18, 0x00, 0x18,
		 								 0x1B, 0x31, 0x35, 0x3F,		 								 		 								 
		 								 0x28, 0x41, 0x06, 0x0C,
		 								 0x0E, 0x13, 0x15, 0x13,
		 								 0x14, 0x11, 0x18, 0x0B,
		 								 0x17, 0x07, 0x12, 0x0B,	
		 								 0x17, 0x07, 0x12}}, 


	{0x39, 0xC0,	2,	{0x0C, 0x17}},		

	{0x39, 0xC6,	2,	{0x08, 0x08}},	

	{0x15, 0xD4,	1,	{0x32}},

	{0x39, 0xD5, 	54,	{0x00, 0x00, 0x00,
		 								 0x00, 0x0A, 0x00, 0x01,
		 								 0x22, 0x00, 0x22, 0x66,
		 								 0x11, 0x01, 0x01, 0x23,
		 								 0x45, 0x67, 0x9A, 0xBC,
		 								 0xAA, 0xBB, 0x45, 0x88,		 								 		 								 
		 								 0x88, 0x88, 0x88, 0x88,
		 								 0x88, 0x88, 0x67, 0x88,
		 								 0x88, 0x08, 0x81, 0x29,
		 								 0x83, 0x88, 0x08, 0x48,	
		 								 0x81, 0x85, 0x28, 0x68,
		 								 0x83, 0x87, 0x88, 0x48,
		 								 0x85, 0x00, 0x00, 0x00,
		 								 0x00, 0x3C, 0x01}},
		 								 
  {0x05, 0x11,	0,	{0x00}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 200, {}},
	
	{0x05, 0x29,	0,	{0x00}},
 	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 5, {}},
	
};


static LCM_setting_table_V3 lcm_deep_sleep_mode_in_setting_V3[] = {
	// Display off sequence
	{0x05,0x28, 0, {0x00}},
	//{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 100, {}},
	//{0x39,0xC3, 3, {0x40, 0x00, 0x20}},

    // Sleep Mode On
	{0x05,0x10, 0, {0x00}},
	{REGFLAG_ESCAPE_ID,REGFLAG_DELAY_MS_V3, 100, {}},

};



// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{
#ifdef BUILD_LK	
		printf("[LK/LCM] lcm_get_params() enter\n");
#else
		printk("[Kernel/LCM] lcm_get_params() enter\n");
#endif
		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE; 

	
		// DSI
		/* Command mode setting */
		//1 Three lane or Four lane
		params->dsi.LANE_NUM				= LCM_FOUR_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.format		= LCM_DSI_FORMAT_RGB888;

		// Video mode setting		
		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		
		params->dsi.vertical_sync_active				= 4;// 3	 2
		params->dsi.vertical_backporch					= 12;// 20   1
		params->dsi.vertical_frontporch 				= 15; // 1   12 
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 


		params->dsi.horizontal_sync_active				= 36; 
		params->dsi.horizontal_backporch				= 118;
		params->dsi.horizontal_frontporch			   	= 118;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
		params->dsi.PLL_CLOCK=267;
		params->dsi.cont_clock = 1;
}

static void lcm_init(void)
{
	
	#ifdef BUILD_LK

    printf("%s,hx8394a_wxga_dsi_cmd_t80p20 LK \n", __func__);
#else
    printk("%s,hx8394a_wxga_dsi_cmd_t80p20 kernel", __func__);
#endif
		lcd_reset(1);
		MDELAY(10);
		lcd_reset(0);
		MDELAY(20);
    /* Hardware reset */
    lcd_reset(1);
    MDELAY(20);
    //lcm_id_pin_handle();
	dsi_set_cmdq_V3(lcm_initialization_setting_V3, sizeof(lcm_initialization_setting_V3) / sizeof(LCM_setting_table_V3), 1);
	
    MDELAY(100);
    lcd_bl_en(1);
		
  
}

static void lcm_suspend(void)
{
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    // when phone sleep , config output low, disable backlight drv chip  
  #ifdef BUILD_LK
    printf("%s,lcm_suspend LK \n", __func__);
#else
    printk("%s,lcm_suspend kernel", __func__);
#endif
        lcd_bl_en(0);
        
	dsi_set_cmdq_V3(lcm_deep_sleep_mode_in_setting_V3, sizeof(lcm_deep_sleep_mode_in_setting_V3) / sizeof(LCM_setting_table_V3), 1);
    //reset low
   	lcd_reset(0);
	

}
static void lcm_resume(void)
{

    //enable VSP & VSN
      #ifdef BUILD_LK
    printf("%s,lcm_resume LK \n", __func__);
#else
    printk("%s,lcm_resume kernel", __func__);
#endif

		lcd_reset(1);
		MDELAY(10);
		lcd_reset(0);
		MDELAY(20);	
		lcd_reset(1);
		MDELAY(20);
    dsi_set_cmdq_V3(lcm_initialization_setting_V3, sizeof(lcm_initialization_setting_V3) / sizeof(LCM_setting_table_V3), 1);
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    //when sleep out, config output high ,enable backlight drv chip  
    MDELAY(100);
    lcd_bl_en(1);

}
static unsigned int lcm_compare_id(void)
{
	int array[4];
int data_array[4];
char buffer[4];
unsigned int id=0;
//read 0xf4=0x94
//write 0xb9 3 parameters
SET_RESET_PIN(1);
MDELAY(20);
SET_RESET_PIN(0);
MDELAY(20);
SET_RESET_PIN(1);
MDELAY(120);
data_array[0]=0x00043902;
data_array[1]=0x9483ffb9;
dsi_set_cmdq(data_array, 2, 1);

array[0] = 0x00013700;// read id return two byte,version and id
dsi_set_cmdq(array, 1, 1);
MDELAY(1);
read_reg_v2(0xf4,buffer,1);
id=buffer[0];
 #ifdef BUILD_LK
		printf("%s, LK t80p20 debug: t80p20 id = 0x%08x\n", __func__, id);
    #else
		printk("%s, kernel t80p20 debug: t80p20 id = 0x%08x\n", __func__, id);
    #endif
if(id == LCM_ID_T80P20)
    	return 1;
else
      return 0;

}
static void lcm_init_power(void)
{
#ifdef BUILD_LK 
		printf("--1->[LK/LCM/hx8394a_wxga_dsi_cmd_t80p20] lcm_init_power() enter\n");		
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	   

#else
		printk("--->[Kernel/LCM/hx8394a_wxga_dsi_cmd_t80p20] lcm_init_power() enter\n");		
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);

#endif
}

static void lcm_suspend_power(void)
{

#ifdef BUILD_LK 
		printf("--->[LK/LCM] lcm_suspend_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ZERO);
		MDELAY(50);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ZERO);	
#else
		printk("--->[Kernel/LCM] lcm_suspend_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ZERO);
		MDELAY(50);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ZERO);
#endif

}

static void lcm_resume_power(void)
{
#ifdef BUILD_LK 
		printf("--->[LK/LCM] lcm_resume_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	    MDELAY(20);
#else
		printk("--->[Kernel/LCM] lcm_resume_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	    MDELAY(20);
#endif

}

static unsigned int	lcm_esd_check(void)
{
#ifndef BUILD_LK
	printk("lcm_esd_check\n\n");
#endif
	return 0;

}
static unsigned int	lcm_esd_recover(void)
{
#ifndef BUILD_LK
	printk("lcm_esd_recover\n\n");
#endif
	return 0;
}

LCM_DRIVER hx8394a_wxga_dsi_vdo_t80p20_lcm_drv = 
{
    .name           = "hx8394a_wxga_dsi_cmd_t80p20",
        .set_util_funcs     = lcm_set_util_funcs,
        .get_params         = lcm_get_params,
        .init               = lcm_init,
        .init_power         = lcm_init_power,
        .suspend            = lcm_suspend,
        .suspend_power      = lcm_suspend_power,
        .resume             = lcm_resume,
        .resume_power       = lcm_resume_power,
        .esd_check          = lcm_esd_check,
        .esd_recover        = lcm_esd_recover,
    .compare_id     = lcm_compare_id,
};

