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


// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------
#define LCM_DSI_CMD_MODE									0
#define FRAME_WIDTH  										(800)
#define FRAME_HEIGHT 										(1280)

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


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

const static unsigned int BL_MIN_LEVEL =20;
static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)    

struct LCM_setting_table {
    unsigned char cmd;
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
//update initial param for IC boe_nt35521 0.01
static struct LCM_setting_table lcm_initialization_setting[] = {
	//CCMON
    {0xFF,	4,  	{0xAA,0x55,0xA5,0x80}},
    
	{0x6F,	2,  	{0x11,0x00}},
	{0xF7,  	2,  	{0x20,0x00}},
	
	{0x6F,  	1,  	{0x06}},
	{0xF7,  	1,  	{0xA0}},
	{0x6F,  	1,  	{0x19}},
	{0xF7,  	1,  	{0x12}},
	/*
	{0x6F,	1,	{0x08}},
	{0xFA,	1,	{0x40}},
	{0x6F,	1,	{0x11}},
	{0xF3,	1,	{0x01}},
	*/
	
	//Page 0
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x00}},
	{0xC8,	1,	{0x80}},
	
	{0xB1,	2,	{0x6C,0x07}},
	
	{0xB6,	1,	{0x08}},
	
	{0x6F,	1,	{0x02}},
	{0xB8,	1,	{0x08}},
	
	{0xBB,	2,	{0x74,0x44}},
	
	{0xBC,  2,  {0x00,0x00}},

	{0xBD,  5,  {0x02,0xB0,0x0C,0x0A,0x00}},
	//Page 1
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x01}},
	{0xB0,	2,	{0x05,0x05}},
	{0xB1,	2,	{0x05,0x05}},
	{0xBC,	2,	{0x90,0x01}},
	{0xBD,	2,	{0x90,0x01}},
	{0xCA,	1,	{0x00}},
	{0xC0,	1,	{0x04}},
	
	{0xB2,  1,  {0x00}},
	{0xBE,	1,	{0x29}},
	
	{0xB3,	2,	{0x37,0x37}},
	{0xB4,	2,	{0x19,0x19}},
	
	{0xB9,	2,	{0x44,0x44}},
	{0xBA,	2,	{0x24,0x24}},

	//Page 2 Gamma
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x02}},
	
	{0xEE,	1,	{0x01}},
	{0xEF,  4,  {0x09,0x06,0x15,0x18}},
	
	{0xB0,  6,  {0x00,0x00,0x00,0x24,0x00,0x55}},
	{0x6F,  1,  {0x06}},	
	{0xB0,  6,  {0x00,0x77,0x00,0x94,0x00,0xC0}},
	{0x6F,  1,  {0x0C}},
	{0xB0,  4,  {0x00,0xE3,0x01,0x1A}},
	{0xB1,  6,  {0x01,0x46,0x01,0x88,0x01,0xBC}},
	{0x6F,  1,  {0x06}},
	{0xB1,  6,  {0x02,0x0B,0x02,0x4B,0x02,0x4D}},
	{0x6F,  1,  {0x0C}},
	{0xB1,  4,  {0x02,0x88,0x02,0xC9}},
	{0xB2,  6,  {0x02,0xF3,0x03,0x29,0x03,0x4E}},
	{0x6F,  1,  {0x06}},
	{0xB2,  6,  {0x03,0x7D,0x03,0x9B,0x03,0xBE}},
	{0x6F,  1,  {0x0C}},
	{0xB2,  4,  {0x03,0xD3,0x03,0xE9}},
	{0xB3,  4,  {0x03,0xFB,0x03,0xFF}},
		
	//Page 6
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x06}},
	{0xB0,	2,	{0x00,0x10}},
	{0xB1,	2,	{0x12,0x14}},
	{0xB2,	2,	{0x16,0x18}},
	{0xB3,	2,	{0x1A,0x29}},
	{0xB4,	2,	{0x2A,0x08}},
	{0xB5,	2,	{0x31,0x31}},
	{0xB6,	2,	{0x31,0x31}},
	{0xB7,	2,	{0x31,0x31}},
	{0xB8,	2,	{0x31,0x0A}},
	{0xB9,	2,	{0x31,0x31}},
	{0xBA,	2,	{0x31,0x31}},
	{0xBB,	2,	{0x0B,0x31}},
	{0xBC,	2,	{0x31,0x31}},
	{0xBD,	2,	{0x31,0x34}},
	{0xBE,	2,	{0x31,0x31}},
	{0xBF,	2,	{0x09,0x2A}},
	{0xC0,	2,	{0x29,0x1B}},
	{0xC1,	2,	{0x19,0x17}},
	{0xC2,	2,	{0x15,0x13}},
	{0xC3,	2,	{0x11,0x01}},
	{0xE5,	2,	{0x31,0x31}},
	{0xC4,	2,	{0x09,0x1B}},
	{0xC5,	2,	{0x19,0x17}},
	{0xC6,	2,	{0x15,0x13}},
	{0xC7,	2,	{0x11,0x29}},
	{0xC8,	2,	{0x2A,0x01}},
	{0xC9,	2,	{0x31,0x31}},
	{0xCA,	2,	{0x31,0x31}},
	{0xCB,	2,	{0x31,0x31}},
	{0xCC,	2,	{0x31,0x0B}},
	{0xCD,	2,	{0x31,0x31}},
	{0xCE,	2,	{0x31,0x31}},
	{0xCF,	2,	{0x0A,0x31}},
	{0xD0,	2,	{0x31,0x31}},
	{0xD1,	2,	{0x31,0x34}},
	{0xD2,	2,	{0x31,0x31}},
	{0xD3,	2,	{0x00,0x2A}},
	{0xD4,	2,	{0x29,0x10}},
	{0xD5,	2,	{0x12,0x14}},
	{0xD6,	2,	{0x16,0x18}},
	{0xD7,	2,	{0x1A,0x08}},
	{0xE6,	2,	{0x31,0x31}},
	{0xD8,	5,	{0x00,0x00,0x00,0x54,0x00}},
	{0xD9,	5,	{0x00,0x15,0x00,0x00,0x00}},
	{0xE7,	1,	{0x00}},
	
	//Page 5
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x03}},
	{0xB0,	2,	{0x20,0x00}},
	{0xB1,	2,	{0x20,0x00}},
	{0xB2,	5,	{0x05,0x00,0x00,0x00,0x00}},
	
	{0xB6,	5,	{0x05,0x00,0x00,0x00,0x00}},
	{0xB7,	5,	{0x05,0x00,0x00,0x00,0x00}},
	
	{0xBA,	5,	{0x57,0x00,0x00,0x00,0x00}},
	{0xBB,	5,	{0x57,0x00,0x00,0x00,0x00}},
	
	{0xC0,	4,	{0x00,0x00,0x00,0x00}},
	{0xC1,	5,	{0x00,0x00,0x00,0x00}},
	
	{0xC4,	1,	{0x60}},
	{0xC5,	1,	{0x40}},
	
	//Page 3
	{0xF0,	5,	{0x55,0xAA,0x52,0x08,0x05}},
	{0xBD,	5,	{0x03,0x01,0x03,0x03,0x03}},
	{0xB0,	2,	{0x17,0x06}},
	{0xB1,	2,	{0x17,0x06}},
	{0xB2,	2,	{0x17,0x06}},
	{0xB3,	2,	{0x17,0x06}},
	{0xB4,	2,	{0x17,0x06}},
	{0xB5,	2,	{0x17,0x06}},
	
	{0xB8,	1,	{0x00}},
	{0xB9,	1,	{0x00}},
	{0xBA,	1,	{0x00}},
	{0xBB,	1,	{0x02}},
	{0xBC,	1,	{0x00}},

	
	{0xC0,	1,	{0x07}},
	
	{0xC4,	1,	{0x80}},
	{0xC5,	1,	{0xA4}},

	{0xC8, 2,   {0x05,0x30}},
	{0xC9, 2,   {0x01,0x31}},
	
	{0xCC, 3,   {0x00,0x00,0x3C}},
	{0xCD, 3,   {0x00,0x00,0x3C}},
	
	{0xD1, 5,   {0x00,0x04,0xFD,0x07,0x10}},
	{0xD2, 5,   {0x00,0x05,0x02,0x07,0x10}},

	{0xE5, 1,   {0x06}},
	{0xE6, 1,   {0x06}},
	{0xE7, 1,   {0x06}},
	{0xE8, 1,   {0x06}},
	{0xE9, 1,   {0x06}},
	{0xEA, 1,   {0x06}},

	{0xED, 1,   {0x30}},
	
	{0x6F,	1,	{0x11}},
	{0xF3,	1,	{0x01}},
	
	{0x35,  3,  {0x00, 0x00, 0x00}},
	
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
    {REGFLAG_DELAY, 20, {}},

    // Sleep Mode On
    {0x10, 1, {0x00}},
    {REGFLAG_DELAY, 120, {}},
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


	memset(params, 0, sizeof(LCM_PARAMS));

    params->type     					= LCM_TYPE_DSI;
    params->width    					= FRAME_WIDTH;
    params->height   					= FRAME_HEIGHT;

    #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
    #else
		params->dsi.mode   = SYNC_EVENT_VDO_MODE;		//SYNC_EVENT_VDO_MODE;
    #endif
	
		// DSI
		/* Command mode setting */
		// Three lane or Four lane
		params->dsi.LANE_NUM								= LCM_FOUR_LANE;
		
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 0;

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count=FRAME_WIDTH*3;
		
    params->dsi.vertical_sync_active		= 28;
    params->dsi.vertical_backporch		    = 4;
    params->dsi.vertical_frontporch 		= 28;
    params->dsi.vertical_active_line		= FRAME_HEIGHT; 

    params->dsi.horizontal_sync_active		= 8;//Jim
    params->dsi.horizontal_backporch		= 26;
    params->dsi.horizontal_frontporch		= 8;
    params->dsi.horizontal_active_pixel 	= FRAME_WIDTH;
		// Bit rate calculation
		// Every lane speed
		//params->dsi.pll_div1=0;				// div1=0,1,2,3;div1_real=1,2,4,4
		//params->dsi.pll_div2=0;				// div2=0,1,2,3;div1_real=1,2,4,4	
		//params->dsi.fbk_div =0x12;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	

    params->dsi.PLL_CLOCK 			= 267;
}

/*to prevent electric leakage*/
#if 0
static void lcm_id_pin_handle(void)
{
    mt_set_gpio_pull_select(GPIO_DISP_ID0_PIN,GPIO_PULL_UP);
    mt_set_gpio_pull_select(GPIO_DISP_ID1_PIN,GPIO_PULL_DOWN);
}
#endif

static void lcm_init(void)
{
	
	#ifdef BUILD_LK
    printf("%s,HE080IA06B_lcm_drv LK \n", __func__);
#else
    printk("%s,HE080IA06B_lcm_drv kernel", __func__);
#endif
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(20);
    /* Hardware reset */
    lcd_reset(1);
    MDELAY(20);
    //lcm_id_pin_handle();
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);  
	
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
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(20);	
	lcd_reset(1);
	MDELAY(20);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    //when sleep out, config output high ,enable backlight drv chip  
    MDELAY(100);
     lcd_bl_en(1);
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK 
		printf("--1->[LK/LCM/HE080IA06B] lcm_init_power() enter\n");		
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//VGP3-3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	   

#else
		printk("--->[Kernel/LCM/HE080IA06B] lcm_init_power() enter\n");		
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


LCM_DRIVER HE080IA06B_lcm_drv =
{
        .name           	= "HE080IA06B",
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
};

/* END PN: , Added by h84013687, 2013.08.13*/
