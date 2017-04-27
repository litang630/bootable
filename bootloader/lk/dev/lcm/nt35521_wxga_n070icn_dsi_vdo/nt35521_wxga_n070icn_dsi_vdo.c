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


const static unsigned char LCD_MODULE_ID = 0x09;//ID0->1;ID1->X
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)
#define LCM_ID_n070icn                                      0x59

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
static struct LCM_setting_table lcm_initialization_setting[] = {
/*
	Note :
	Data ID will depends on the following rule.	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05
	Structure Format :
	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},
	...
	Setting ending by predefined flag
	*/
	
	//specific configuration
	{0xFF,4,{0xAA,0x55,0xA5,0x80}},
	
	{0x6F,2,{0x11,0x00}},
	{0xF7,2,{0x20,0x00}},
	{0x6F,1,{0x06}},
	{0xF7,1,{0xA0}},
	{0x6F,1,{0x19}},
	{0xF7,1,{0x12}},
	{0x6F,1,{0x08}},
	{0xFA,1,{0x40}},
	{0x6F,1,{0x11}},
	{0xF3,1,{0x01}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x00}},
	{0xC8,1,{0x80}},
	{0xB1,2,{0x68,0x01}},
	{0xB6,1,{0x08}},
	{0x6F,1,{0x02}},
	{0xB8,1,{0x08}},
	{0xBB,2,{0x54,0x54}},
	{0xBC,2,{0x05,0x05}},
	{0xC7,1,{0x01}},
	{0xBD,5,{0x02,0xB0,0x0C,0x0A,0x00}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x01}},
	{0xB0,2,{0x05,0x05}},
	{0xB1,2,{0x05,0x05}},
	{0xBC,2,{0x3A,0x01}},
	{0xBD,2,{0x3E,0x01}},
	{0xCA,1,{0x00}},
	{0xC0,1,{0x04}},
	{0xBE,1,{0x80}},
	{0xB3,2,{0x19,0x19}},
	{0xB4,2,{0x12,0x12}},
	{0xB9,2,{0x24,0x24}},
	{0xBA,2,{0x14,0x14}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x02}},
	{0xEE,1,{0x02}},
	{0xEF,4,{0x09,0x06,0x15,0x18}},
	{0xB0,6,{0x00,0x00,0x00,0x08,0x00,0x17}},
	{0x6F,1,{0x06}},
	{0xB0,6,{0x00,0x25,0x00,0x30,0x00,0x45}},
	{0x6F,1,{0x0C}},
	{0xB0,4,{0x00,0x56,0x00,0x7A}},
	{0xB1,6,{0x00,0xA3,0x00,0xE7,0x01,0x20}},
	{0x6F,1,{0x06}},
	{0xB1,6,{0x01,0x7A,0x01,0xC2,0x01,0xC5}},
	{0x6F,1,{0x0C}},
	{0xB1,4,{0x02,0x06,0x02,0x5F}},
	{0xB2,6,{0x02,0x92,0x02,0xD0,0x02,0xFC}},
	{0x6F,1,{0x06}},
	{0xB2,6,{0x03,0x35,0x03,0x5D,0x03,0x8B}},
	{0x6F,1,{0x0C}},
	{0xB2,4,{0x03,0xA2,0x03,0xBF}},
	{0xB3,4,{0x03,0xE8,0x03,0xFF}},
	{0xBC,6,{0x00,0x00,0x00,0x08,0x00,0x18}},
	{0x6F,1,{0x06}},
	{0xBC,6,{0x00,0x27,0x00,0x32,0x00,0x49}},
	{0x6F,1,{0x0C}},
	{0xBC,4,{0x00,0x5C,0x00,0x83}},
	{0xBD,6,{0x00,0xAF,0x00,0xF3,0x01,0x2A}},
	{0x6F,1,{0x06}},
	{0xBD,6,{0x01,0x84,0x01,0xCA,0x01,0xCD}},
	{0x6F,1,{0x0C}},
	{0xBD,4,{0x02,0x0E,0x02,0x65}},
	{0xBE,6,{0x02,0x98,0x02,0xD4,0x03,0x00}},
	{0x6F,1,{0x06}},
	{0xBE,6,{0x03,0x37,0x03,0x5F,0x03,0x8D}},
	{0x6F,1,{0x0C}},
	{0xBE,4,{0x03,0xA4,0x03,0xBF}},
	{0xBF,4,{0x03,0xE8,0x03,0xFF}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x06}},
	{0xB0,2,{0x00,0x17}},
	{0xB1,2,{0x16,0x15}},
	{0xB2,2,{0x14,0x13}},
	{0xB3,2,{0x12,0x11}},
	{0xB4,2,{0x10,0x2D}},
	{0xB5,2,{0x01,0x08}},
	{0xB6,2,{0x09,0x31}},
	{0xB7,2,{0x31,0x31}},
	{0xB8,2,{0x31,0x31}},
	{0xB9,2,{0x31,0x31}},
	{0xBA,2,{0x31,0x31}},
	{0xBB,2,{0x31,0x31}},
	{0xBC,2,{0x31,0x31}},
	{0xBD,2,{0x31,0x09}},
	{0xBE,2,{0x08,0x01}},
	{0xBF,2,{0x2D,0x10}},
	{0xC0,2,{0x11,0x12}},
	{0xC1,2,{0x13,0x14}},
	{0xC2,2,{0x15,0x16}},
	{0xC3,2,{0x17,0x00}},
	{0xE5,2,{0x31,0x31}},
	{0xC4,2,{0x00,0x17}},
	{0xC5,2,{0x16,0x15}},
	{0xC6,2,{0x14,0x13}},
	
	//add 
//	{0xC7,2,{0x12,0x11}},
//	{0xC8,2,{0x10,0x2D}},
//	{0xC9,2,{0x01,0x08}},
	//add 
	
	{0xCA,2,{0x09,0x31}},
	{0xCB,2,{0x31,0x31}},
	{0xCC,2,{0x31,0x31}},
	{0xCD,2,{0x31,0x31}},
	{0xCE,2,{0x31,0x31}},
	{0xCF,2,{0x31,0x31}},
	{0xD0,2,{0x31,0x31}},
	{0xD1,2,{0x31,0x09}},
	{0xD2,2,{0x08,0x01}},
	{0xD3,2,{0x2D,0x10}},
	{0xD4,2,{0x11,0x12}},
	{0xD5,2,{0x13,0x14}},
	{0xD6,2,{0x15,0x16}},
	{0xD7,2,{0x17,0x00}},
	{0xE6,2,{0x31,0x31}},
	{0xD8,5,{0x00,0x00,0x00,0x00,0x00}},
	{0xD9,5,{0x00,0x00,0x00,0x00,0x00}},
	{0xE7,1,{0x00}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x03}},
	{0xB0,2,{0x20,0x00}},
	{0xB1,2,{0x20,0x00}},
	{0xB2,5,{0x05,0x00,0x42,0x00,0x00}},
	{0xB6,5,{0x05,0x00,0x42,0x00,0x00}},
	{0xBA,5,{0x53,0x00,0x42,0x00,0x00}},
	{0xBB,5,{0x53,0x00,0x42,0x00,0x00}},
	{0xC4,1,{0x40}},
	
	
	{0xF0,5,{0x55,0xAA,0x52,0x08,0x05}},
	{0xB0,2,{0x17,0x06}},
	{0xB8,1,{0x00}},
	{0xBD,5,{0x03,0x01,0x01,0x00,0x01}},
	{0xB1,2,{0x17,0x06}},
	{0xB9,2,{0x00,0x01}},
	{0xB2,2,{0x17,0x06}},
	{0xBA,2,{0x00,0x01}},
	{0xB3,2,{0x17,0x06}},
	{0xBB,2,{0x0A,0x00}},
	{0xB4,2,{0x17,0x06}},
	{0xB5,2,{0x17,0x06}},
	{0xB6,2,{0x14,0x03}},
	{0xB7,2,{0x00,0x00}},
	{0xBC,2,{0x02,0x01}},
	{0xC0,1,{0x05}},
	{0xC4,1,{0xA5}},
	{0xC8,2,{0x03,0x30}},//1
	{0xC9,2,{0x03,0x51}},//1
	{0xD1,5,{0x00,0x05,0x03,0x00,0x00}},
	{0xD2,5,{0x00,0x05,0x09,0x00,0x00}},
	{0xE5,1,{0x02}},
	{0xE6,1,{0x02}},
	{0xE7,1,{0x02}},
	{0xE9,1,{0x02}},
	{0xED,1,{0x33}},
// add 	
	{0x6F,1,{0x11}},
	{0xF3,1,{0x01}},
	{0x35,1,{0x00}},	
	//add 
//	{0x3a,1,{0x77}},
//add 								
	
// Normal Display
{0x11,	1,	{0x00}},
{REGFLAG_DELAY, 120, {}},
	
{0x29,	1,	{0x00}},
{REGFLAG_DELAY, 20, {}},
	
{REGFLAG_END_OF_TABLE, 0x00, {}},
};

static struct LCM_setting_table lcm_sleep_out_setting[] = {
    //Sleep Out
    {0x11, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},

    // Display ON
    {0x29, 1, {0x00}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = {
    // Display off sequence
    {0x28, 1, {0x00}},
    {REGFLAG_DELAY, 10, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 50, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd) {
            case REGFLAG_DELAY :
                if(table[i].count <= 10)
                    MDELAY(table[i].count);
                else
                    MDELAY(table[i].count);
                break;

            case REGFLAG_END_OF_TABLE :
                break;

            default:
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
        }
    }
}



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
		
		params->dsi.vertical_sync_active				= 2;// 3	 2
		params->dsi.vertical_backporch					= 14;// 20   1
		params->dsi.vertical_frontporch 				= 16; // 1   12 
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 


		params->dsi.horizontal_sync_active				= 2; 
		params->dsi.horizontal_backporch				= 34;
		params->dsi.horizontal_frontporch			   	= 24;
		params->dsi.horizontal_active_pixel 			= FRAME_WIDTH;
		params->dsi.PLL_CLOCK=236;

 		params->dsi.noncont_clock = 1;
 		params->dsi.noncont_clock_period= 2;
		
}

static void lcm_init(void)
{
	
#ifdef BUILD_LK

    printf("%s,lcm_init n070icn LK \n", __func__);
#else
    printk("%s,lcm_init n070icn kernel", __func__);
#endif
    /* Power supply(VDD=1.8V, VCI=3.3V) */
		//lcd_power_en(1);
    // MDELAY(10);
		lcd_reset(0);
		MDELAY(45);	
		lcd_reset(1);
		MDELAY(10); 
		lcd_reset(0);
		MDELAY(10);
		lcd_reset(1);
		MDELAY(30);
    //lcm_id_pin_handle();
	  push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    //MDELAY(100);
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
    push_table(lcm_deep_sleep_mode_in_setting, sizeof(lcm_deep_sleep_mode_in_setting) / sizeof(struct LCM_setting_table), 1);   
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
    MDELAY(10);
		lcd_reset(0);
		MDELAY(45);	
		lcd_reset(1);
		MDELAY(10); 
		lcd_reset(0);
		MDELAY(10);
		lcd_reset(1);
		MDELAY(30);

		push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    //when sleep out, config output high ,enable backlight drv chip  
}
static unsigned int lcm_compare_id(void)
{
	unsigned int id=0;
	unsigned char buffer[2];
	unsigned int array[16];  
	#ifdef BUILD_LK
		printf("%s, LK n070icn debug: n070icn \n", __func__);
    #else
		printk("%s, kernel n070icn debug: n070icn \n", __func__);
    #endif
	lcd_reset(0);
	MDELAY(45);	
	lcd_reset(1);
	MDELAY(10); 
	lcd_reset(0);
	MDELAY(10);
	lcd_reset(1);
	MDELAY(30);

	array[0] = 0x00023700;   // read id return two byte,version and id
	dsi_set_cmdq(array, 1, 1);
	
	read_reg_v2(0xF4, buffer, 2);
	id = buffer[0]; //we only need ID
    #ifdef BUILD_LK
		printf("%s, LK n070icn debug: n070icn id = 0x%08x\n", __func__, id);
    #else
		printk("%s, kernel n070icn debug: n070icn id = 0x%08x\n", __func__, id);
    #endif

    if(id == LCM_ID_n070icn)
    	return 1;
    else
        return 0;


}
static void lcm_init_power(void)
{
#ifdef BUILD_LK 
		printf("--1->[LK/LCM/nt35521_wxga_n070icn_dsi_vdo] lcm_init_power() enter\n");		
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	   

#else
		printk("--->[Kernel/LCM/nt35521_wxga_n070icn_dsi_vdo] lcm_init_power() enter\n");		
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

LCM_DRIVER nt35521_wxga_n070icn_dsi_vdo_lcm_drv = 
{
    .name           = "nt35521_wxga_n070icn_dsi_vdo",
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
 //   .compare_id     = lcm_compare_id,
};

