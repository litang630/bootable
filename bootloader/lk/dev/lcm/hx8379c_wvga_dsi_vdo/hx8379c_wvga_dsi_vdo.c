
#ifdef BUILD_LK
#include <string.h>
#include <mt_gpio.h>
#else
#include <linux/string.h>
#if defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif
#endif
#include "lcm_drv.h"
#include <cust_gpio_usage.h>

#include <string.h>

#if defined(BUILD_LK)
#define LCM_PRINT printf
#elif defined(BUILD_UBOOT)
#define LCM_PRINT printf
#else
#define LCM_PRINT printk
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH (480) // pixel
#define FRAME_HEIGHT (854)// (800) // pixel

#define REGFLAG_DELAY 0xAB
#define REGFLAG_END_OF_TABLE 0xAA // END OF REGISTERS MARKER

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};
 unsigned int hx_id=0x0c;

#define SET_RESET_PIN(v) (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update) lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update) lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd) lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums) lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd) lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size) lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)






// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS * util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS * params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type = LCM_TYPE_DSI;

	params->width = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	// enable tearing-free
	params->dbi.te_mode = LCM_DBI_TE_MODE_DISABLED;
	params->dbi.te_edge_polarity = LCM_POLARITY_RISING;

	params->dsi.mode = SYNC_PULSE_VDO_MODE;

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM = LCM_TWO_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format = LCM_DSI_FORMAT_RGB888;

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size = 256;

	// Video mode setting	
	params->dsi.PS = LCM_PACKED_PS_24BIT_RGB888;

	params->dsi.vertical_sync_active = 6;
	params->dsi.vertical_backporch = 5;
	params->dsi.vertical_frontporch = 6;
	params->dsi.vertical_active_line = FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active = 10;
	params->dsi.horizontal_backporch =5;// 5;
	params->dsi.horizontal_frontporch = 8;//8;
	params->dsi.horizontal_active_pixel = FRAME_WIDTH;

	// Bit rate calculation
	//params->dsi.pll_div1 = 29; // fref=26MHz, fvco=fref*(div1+1)	(div1=0~63, fvco=500MHZ~1GHz)
	//params->dsi.pll_div2 = 1; // div2=0~15: fout=fvo/(2*div2)
	//params->dsi.fbk_div = 434;


	params->dsi.PLL_CLOCK = 160;//182; //260; ///////params->dsi.PLL_CLOCK = 234£¬±íÊ¾234MHZ
}

static void init_lcm_registers(void)
{
	unsigned int data_array[16];
	//static unsigned int data_temp=0x30;

	data_array[0] = 0x00043902; // SET EXTC
	data_array[1] = 0x7983ffb9;
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00153902; //SET POWER
	data_array[1] = 0x181866b1;
	data_array[2] = 0xd0909131;
	data_array[3] = 0x3880d8ec;
	data_array[4] = 0x444334f8;	
	data_array[5] = 0x00308000;
	
	dsi_set_cmdq(data_array, 6, 1);

	data_array[0] = 0x000a3902; //SET DISP
	data_array[1] = 0x0bfe80b2;
	//data_array[2] = 0x11503004;
	data_array[2] = 0x11505004;
	data_array[3] = 0x00001d42;
	//data_array[4] = 0x00002019;
	dsi_set_cmdq(data_array, 4, 1);

	data_array[0] = 0x000b3902; //SET CYC
	data_array[1] = 0x008000b4;
	data_array[2] = 0x08860086;
	data_array[3] = 0x00860886;
	 
	dsi_set_cmdq(data_array, 4, 1);

	
	data_array[0] = 0x00023902; //SET PANEL
	data_array[1] = 0x00000ecc;
	dsi_set_cmdq(data_array, 2, 1);
	
	data_array[0] = 0x00023902; //	// Set Offset 
	data_array[1] = 0x000033d2;
	dsi_set_cmdq(data_array, 2, 1);						

	data_array[0] = 0x001e3902; //SET GIP0 SIGNAL
	data_array[1] = 0x000700d3;
	data_array[2] = 0x06060000;
	data_array[3] = 0x00061032;
	data_array[4] = 0x03700306;
	data_array[5] = 0x00080070;
	data_array[6] = 0x06111108;
	data_array[7] = 0x06061306;
	data_array[8] = 0x00000913;
	
	dsi_set_cmdq(data_array, 9, 1);
	
	data_array[0] = 0x00213902; //SET GIP1 SIGNAL
	data_array[1] = 0x181818d5;
	data_array[2] = 0x02010018;
	data_array[3] = 0x19212003;
	data_array[4] = 0x18181819;
	data_array[5] = 0x18181818;
	data_array[6] = 0x18181818;
	data_array[7] = 0x18181818;
	data_array[8] = 0x18181818;
	data_array[9] = 0x00000018;
	
	dsi_set_cmdq(data_array, 10, 1);
	
	data_array[0] = 0x00213902; //SET GIP2 SIGNAL
	data_array[1] = 0x191818d6;
	data_array[2] = 0x03000119;
	data_array[3] = 0x18202102;
	data_array[4] = 0x18181818;
	data_array[5] = 0x18181818;
	data_array[6] = 0x18181818;
	data_array[7] = 0x18181818;
	data_array[8] = 0x18181818;
	data_array[9] = 0x00000018;
	
	dsi_set_cmdq(data_array, 10, 1);



	data_array[0] = 0x002b3902; //Set GAMMA
	//data_array[1] = 0x000000e0;
	data_array[1] = 0x231f00e0;
	data_array[2] = 0x293f342e;
	data_array[3] = 0x0d0b0740;
	data_array[4] = 0x14120f17;
	data_array[5] = 0x11061412;
	data_array[6] = 0x20001712;
	data_array[7] = 0x3f342e24;
	data_array[8] = 0x09073f2a;
	data_array[9] = 0x120d170b;
	data_array[10] = 0x06131214;
	data_array[11] = 0x00181311;
	
	dsi_set_cmdq(data_array, 12, 1);

	data_array[0] = 0x00033902; //SET VCOM
	data_array[1] = 0x003434b6;
	//data_array[1] =(0x000000b6|(data_temp<<8))|(data_temp<<16);
	
	dsi_set_cmdq(data_array, 2, 1);

	data_array[0] = 0x00023902; //SET add mode
	data_array[1] = 0x00000236;
	dsi_set_cmdq(data_array, 2, 1);
	
	data_array[0] = 0x00110500;   // Sleep Out 
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00290500;  // Display On 
	dsi_set_cmdq(data_array, 1, 1);

	
}

static void lcm_init(void)
{

#ifdef BUILD_UBOOT 
#elif defined(BUILD_LK) 
#else
	hwPowerOn(MT6328_POWER_LDO_VCAMD, VOL_1800, "1V8_LCD_VIO");
	hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_2800, "2V8_LCD_VCI");
	MDELAY(5);
	hwPowerDown(MT6328_POWER_LDO_VCAMA, "2V8_LCD_VCI");
	MDELAY(5);
	hwPowerOn(MT6328_POWER_LDO_VCAMA, VOL_2800, "2V8_LCD_VCI" );
#endif

	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(100);

	init_lcm_registers();
}

static void lcm_suspend(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00280500;   
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	data_array[0] = 0x00100500;    
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);

	mt_set_gpio_mode( GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir( GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(120);

#if 0
#ifdef BUILD_UBOOT
#elif defined(BUILD_LK)
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_01);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(120);
#else

	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_01);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
	MDELAY(10);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(120);

	hwPowerDown(MT6328_POWER_LDO_VCAMA, "2V8_LCD_VCI");
	hwPowerDown(MT6328_POWER_LDO_VCAMD, "1V8_LCD_VIO");

	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
#endif
#endif
}

static void lcm_resume(void)
{
	
	mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_RST, GPIO_PULL_ENABLE);
	mt_set_gpio_dir( GPIO_LCM_RST, GPIO_DIR_OUT);
	mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
	MDELAY(150);

	lcm_init();	
	MDELAY(15);	
}
/*
#if (!defined(BUILD_UBOOT) && !defined(BUILD_LK))
static void lcm_esd_recover(void)
{
	lcm_suspend();
	lcm_resume();
}
#endif
*/

static unsigned int lcm_compare_id(void)
{
	int status = 0;

	unsigned int id,id1=0;
	unsigned char buffer[3];
	unsigned int array[16];  

	SET_RESET_PIN(1); 
	MDELAY(10); 
	SET_RESET_PIN(0); 
	MDELAY(50); 
	SET_RESET_PIN(1); 
	MDELAY(120); 

	
	array[0] = 0x00043902; // SET EXTC
	array[1] = 0x7983ffb9;
	dsi_set_cmdq(array, 2, 1);
	
	read_reg_v2(0xdc, buffer, 1);  // 0x0c
	id = buffer[0]; //we only need ID  
	LCM_PRINT("hx8379--lk---id1=0x%02x---\n",buffer[0]);
	return id;
}

// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hx8379c_wvga_dsi_vdo_drv = {
	.name = "hx8379c_wvga_dsi_vdo_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.compare_id = lcm_compare_id,
	.init = lcm_init,
	.suspend = lcm_suspend,
	.resume = lcm_resume,
	//.set_backlight = lcm_setbacklight,
	//.set_pwm = lcm_setpwm,
	//.get_pwm = lcm_getpwm,
	//.update = lcm_update,
//#if (!defined(BUILD_UBOOT) && !defined(BUILD_LK))
//	.esd_recover = lcm_esd_recover,
//#endif
};

