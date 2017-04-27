#include <platform/mt_typedefs.h>
#include <platform/mt_reg_base.h> 
#include <platform/mt_pmic.h>
//#include <platform/eta6005.h>
#include <printf.h>

#if !defined(CONFIG_POWER_EXT)
#include <platform/upmu_common.h>
#endif

int g_eta6005_log_en=0;

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define PRECC_BATVOL 2800 //preCC 2.8V
/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/


void eta6005_hw_init(void)
{
    //pull CE low
    #if defined(GPIO_CHR_CE_PIN)
    mt_set_gpio_mode(GPIO_CHR_CE_PIN,GPIO_MODE_GPIO);  
    mt_set_gpio_dir(GPIO_CHR_CE_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CHR_CE_PIN,GPIO_OUT_ZERO);    
    #endif
}

static CHARGER_TYPE g_chr_type_num = CHARGER_UNKNOWN;
int hw_charging_get_charger_type(void);

void eta6005_charging_enable(kal_uint32 bEnable)
{
    int temp_CC_value = 0;
    kal_int32 bat_val = 0;

    if(CHARGER_UNKNOWN == g_chr_type_num && KAL_TRUE == upmu_is_chr_det())
    {
        hw_charging_get_charger_type();
        dprintf(CRITICAL, "[BATTERY:eta6005] charger type: %d\n", g_chr_type_num);
    }

    bat_val = get_i_sense_volt(1);
    if (g_chr_type_num == STANDARD_CHARGER)
    {
        temp_CC_value = 2000;
  			dprintf(INFO, "--->eta temp_CC_value=2000 \n");
    }
    else if (g_chr_type_num == STANDARD_HOST \
        || g_chr_type_num == CHARGING_HOST \
        || g_chr_type_num == NONSTANDARD_CHARGER)
    {
        temp_CC_value = 500;
				dprintf(INFO, "--->eta temp_CC_value=500 \n");  //Fast Charging Current Limit at 500mA
    }
    else
    {
        dprintf(INFO, "[BATTERY:eta6005] Unknown charger type\n");
        temp_CC_value = 500;
				dprintf(INFO, "--->eta temp_CC_value=500 \n");
    }
      	

    dprintf(INFO, "[BATTERY:eta6005] eta6005_set_ac_current(), CC value(%dmA) \r\n", temp_CC_value);
    dprintf(INFO, "[BATTERY:eta6005] charger enable/disable %d !\r\n", bEnable);
}

#if defined(CONFIG_POWER_EXT)

int hw_charging_get_charger_type(void)
{
    g_chr_type_num = STANDARD_HOST;

    return STANDARD_HOST;
}

#else

extern void Charger_Detect_Init(void);
extern void Charger_Detect_Release(void);
extern void mdelay (unsigned long msec);
extern kal_uint16 bc11_set_register_value(PMU_FLAGS_LIST_ENUM flagname,kal_uint32 val);

static void hw_bc11_dump_register(void)
{
/*
	kal_uint32 reg_val = 0;
	kal_uint32 reg_num = CHR_CON18;
	kal_uint32 i = 0;

	for(i=reg_num ; i<=CHR_CON19 ; i+=2)
	{
		reg_val = upmu_get_reg_value(i);
		dprintf(INFO, "Chr Reg[0x%x]=0x%x \r\n", i, reg_val);
	}	
*/
}

static void hw_bc11_init(void)
{
    mdelay(200);
    Charger_Detect_Init();

    //RG_bc11_BIAS_EN=1
    bc11_set_register_value(PMIC_RG_BC11_BIAS_EN,1);
    //RG_bc11_VSRC_EN[1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VSRC_EN,0);
    //RG_bc11_VREF_VTH = [1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0);
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0);
    //RG_bc11_IPU_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0);
    //RG_bc11_IPD_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0);
    //bc11_RST=1
    bc11_set_register_value(PMIC_RG_BC11_RST,1);
    //bc11_BB_CTRL=1
    bc11_set_register_value(PMIC_RG_BC11_BB_CTRL,1);

    mdelay(50);

    if(g_eta6005_log_en>1)
    {
        dprintf(INFO, "hw_bc11_init() \r\n");
        hw_bc11_dump_register();
    }	
}
 
static U32 hw_bc11_DCD(void)
{
    U32 wChargerAvail = 0;

    //RG_bc11_IPU_EN[1.0] = 10
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0x2);  
    //RG_bc11_IPD_EN[1.0] = 01
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x1);
    //RG_bc11_VREF_VTH = [1:0]=01
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x1);
    //RG_bc11_CMP_EN[1.0] = 10
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x2);

    mdelay(80);

    wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

    if(g_eta6005_log_en>1)
    {
        dprintf(INFO, "hw_bc11_DCD() \r\n");
        hw_bc11_dump_register();
    }

    //RG_bc11_IPU_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0x0);
    //RG_bc11_IPD_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x0);
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x0);
    //RG_bc11_VREF_VTH = [1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x0);

    return wChargerAvail;
}

static U32 hw_bc11_stepA1(void)
{
    U32 wChargerAvail = 0;
      
    //RG_bc11_IPD_EN[1.0] = 01
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x1);
    //RG_bc11_VREF_VTH = [1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x0);
    //RG_bc11_CMP_EN[1.0] = 01
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x1);

    mdelay(80);

    wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

    if(g_eta6005_log_en>1)
    {
    	dprintf(INFO, "hw_bc11_stepA1() \r\n");
    	hw_bc11_dump_register();
    }

    //RG_bc11_IPD_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x0);
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x0);

    return  wChargerAvail;
}


static U32 hw_bc11_stepA2(void)
{
    U32 wChargerAvail = 0;

    //RG_bc11_VSRC_EN[1.0] = 10 
    bc11_set_register_value(PMIC_RG_BC11_VSRC_EN,0x2);
    //RG_bc11_IPD_EN[1:0] = 01
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x1);
    //RG_bc11_VREF_VTH = [1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x0);
    //RG_bc11_CMP_EN[1.0] = 01
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x1);

    mdelay(80);

    wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

    if(g_eta6005_log_en)
    {
    	dprintf(INFO, "hw_bc11_stepB1() \r\n");
    	hw_bc11_dump_register();
    }

    //RG_bc11_VSRC_EN[1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VSRC_EN,0x0);
    //RG_bc11_IPD_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x0);
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x0);

    return  wChargerAvail;
}


static U32 hw_bc11_stepB2(void)
{
    U32 wChargerAvail = 0;
      
    //RG_bc11_IPU_EN[1:0]=10
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0x2); 
    //RG_bc11_VREF_VTH = [1:0]=01
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x1);
    //RG_bc11_CMP_EN[1.0] = 01
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x1);

    mdelay(80);

    wChargerAvail = bc11_get_register_value(PMIC_RGS_BC11_CMP_OUT);

    if(g_eta6005_log_en)
    {
    	dprintf(INFO, "hw_bc11_stepB2() \r\n");
    	hw_bc11_dump_register();
    }

    if (!wChargerAvail)
    {
        //RG_bc11_VSRC_EN[1.0] = 10 
        //mt6325_upmu_set_rg_bc11_vsrc_en(0x2);
        bc11_set_register_value(PMIC_RG_BC11_VSRC_EN,0x2); 
    }
    //RG_bc11_IPU_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0x0); 
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x0); 
    //RG_bc11_VREF_VTH = [1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x0); 

    return  wChargerAvail;
}


static void hw_bc11_done(void)
{
    //RG_bc11_VSRC_EN[1:0]=00
    bc11_set_register_value(PMIC_RG_BC11_VSRC_EN,0x0);
    //RG_bc11_VREF_VTH = [1:0]=0
    bc11_set_register_value(PMIC_RG_BC11_VREF_VTH,0x0);
    //RG_bc11_CMP_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_CMP_EN,0x0);
    //RG_bc11_IPU_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPU_EN,0x0);
    //RG_bc11_IPD_EN[1.0] = 00
    bc11_set_register_value(PMIC_RG_BC11_IPD_EN,0x0);
    //RG_bc11_BIAS_EN=0
    bc11_set_register_value(PMIC_RG_BC11_BIAS_EN,0x0);

    Charger_Detect_Release();

    if(g_eta6005_log_en>1)
    {
    	dprintf(INFO, "hw_bc11_done() \r\n");
    	hw_bc11_dump_register();
    }
}

int hw_charging_get_charger_type(void)
{
    if(CHARGER_UNKNOWN != g_chr_type_num)
        return g_chr_type_num;
#if 0
	  return STANDARD_HOST;
#else
    CHARGER_TYPE CHR_Type_num = CHARGER_UNKNOWN;
    
    /********* Step initial  ***************/         
    hw_bc11_init();
 
    /********* Step DCD ***************/  
    if(1 == hw_bc11_DCD())
    {
         /********* Step A1 ***************/
         if(1 == hw_bc11_stepA1())
         {             
             CHR_Type_num = APPLE_2_1A_CHARGER;
             dprintf(INFO, "step A1 : Apple 2.1A CHARGER!\r\n");
         }
         else
         {
             CHR_Type_num = NONSTANDARD_CHARGER;
             dprintf(INFO, "step A1 : Non STANDARD CHARGER!\r\n");
         }
    }
    else
    {
         /********* Step A2 ***************/
         if(1 == hw_bc11_stepA2())
         {
             /********* Step B2 ***************/
             if(1 == hw_bc11_stepB2())
             {
                 CHR_Type_num = STANDARD_CHARGER;
                 dprintf(INFO, "step B2 : STANDARD CHARGER!\r\n");
             }
             else
             {
                 CHR_Type_num = CHARGING_HOST;
                 dprintf(INFO, "step B2 :  Charging Host!\r\n");
             }
         }
         else
         {
             CHR_Type_num = STANDARD_HOST;
             dprintf(INFO, "step A2 : Standard USB Host!\r\n");
         }
    }
 
    /********* Finally setting *******************************/
    hw_bc11_done();

    g_chr_type_num = CHR_Type_num;

    return g_chr_type_num;
#endif    
}
#endif
