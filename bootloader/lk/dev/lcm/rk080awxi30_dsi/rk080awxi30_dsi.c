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

       
//update initial param 
static struct LCM_setting_table lcm_initialization_setting[] = 
{
    {0xF0,2,{0x5A,0x5A}},
    {0xD0,2,{0x00,0x10}},

    {0x11,	0,	{}},
    {REGFLAG_DELAY, 120, {}},
    
    {0xC3,3,{0x40,0x00,0x28}},
    
    {0x29,	0,	{}},
    {REGFLAG_DELAY, 20, {}},
    
    {REGFLAG_END_OF_TABLE, 0x00, {}},
};


static struct LCM_setting_table lcm_sleep_out_setting[] = 
{
    //Sleep Out
    {0x11, 0, {}},
    {REGFLAG_DELAY, 120, {}},
    
    // Display ON
    {0x29, 0, {}},
    {REGFLAG_DELAY, 20, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_deep_sleep_mode_in_setting[] = 
{
    // Display off sequence
    {0x28, 0, {}},
    {REGFLAG_DELAY, 20, {}},
    
    // Sleep Mode On
    {0x10, 0, {0}},
    {REGFLAG_DELAY, 120, {}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i,j,buffer[6];
   
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
    
    params->type   = LCM_TYPE_DSI;
    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

#if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
#else
    params->dsi.mode   = SYNC_EVENT_VDO_MODE;
#endif

    // DSI
    /* Command mode setting */
    // Three lane or Four lane
    params->dsi.LANE_NUM				= LCM_FOUR_LANE;
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
    
    //params->dsi.ufoe_enable = 1;
    //params->dsi.ufoe_params.vlc_disable = 1;
    
    params->dsi.vertical_sync_active				= 4;	
    params->dsi.vertical_backporch					= 8;	
    params->dsi.vertical_frontporch					= 8;	
    params->dsi.vertical_active_line				= FRAME_HEIGHT; 
    
    params->dsi.horizontal_sync_active				= 4;	
    params->dsi.horizontal_backporch				= 132;	
    params->dsi.horizontal_frontporch				= 24;	
    params->dsi.horizontal_active_pixel				= FRAME_WIDTH;
    
    params->dsi.PLL_CLOCK = 220;
    params->dsi.cont_clock = 1;
}


static void lcm_init(void)
{
	
	#ifdef BUILD_LK
    printf("%s,rk080awxi30_dsi LK \n", __func__);
#else
    printk("%s,rk080awxi30_dsi kernel", __func__);
#endif
  lcd_reset(0);
  MDELAY(40);
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(10);
    /* Hardware reset */
  lcd_reset(1);
  MDELAY(120);
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
  lcd_reset(0);
  MDELAY(40);
	lcd_reset(1);
	MDELAY(10);
	lcd_reset(0);
	MDELAY(10);
    /* Hardware reset */
  lcd_reset(1);
  MDELAY(120);
    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    //Back to MP.P7 baseline , solve LCD display abnormal On the right
    //when sleep out, config output high ,enable backlight drv chip  
    //MDELAY(100);
}

static void lcm_init_power(void)
{
#ifdef BUILD_LK 
		printf("--1->[LK/LCM/rk080awxi30_dsi] lcm_init_power() enter\n");	
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(5);
		//3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
		MDELAY(45);
	   

#else
		printk("--->[Kernel/LCM/rk080awxi30_dsi] lcm_init_power() enter\n");	
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(5);
		//3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
		MDELAY(45);

#endif
}

static void lcm_suspend_power(void)
{

#ifdef BUILD_LK 
		printf("--->[LK/LCM] lcm_suspend_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ZERO);
		MDELAY(5);
		//3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ZERO);	
#else
		printk("--->[Kernel/LCM] lcm_suspend_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ZERO);
		MDELAY(5);
		//3.3V
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
		//3.3V
		lcm_set_gpio_output(GPIO_LCD_V33_EN, GPIO_OUT_ONE);
	    MDELAY(20);
#else
		printk("--->[Kernel/LCM] lcm_resume_power() enter\n");
		//1.8V
		lcm_set_gpio_output(GPIO_LCD_V18_EN, GPIO_OUT_ONE);
		MDELAY(20);
		//3.3V
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
LCM_DRIVER rk080awxi30_dsi_lcm_drv =
{
    .name           	= "rk080awxi30_dsi",
    .set_util_funcs 	= lcm_set_util_funcs,
    .get_params     	= lcm_get_params,
    .init           	= lcm_init,
    .init_power         = lcm_init_power,
    .suspend        	= lcm_suspend,
        .suspend_power      = lcm_suspend_power,
        .resume             = lcm_resume,
        .resume_power       = lcm_resume_power,
        .esd_check          = lcm_esd_check,
        .esd_recover        = lcm_esd_recover,
};
/* END PN: , Added by h84013687, 2013.08.13*/
