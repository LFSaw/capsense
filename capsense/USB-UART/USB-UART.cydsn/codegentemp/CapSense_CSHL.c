/*******************************************************************************
* File Name: CapSense_CSHL.c
* Version 2.0
*
* Description:
*  This file provides the source code to the High Level APIs for the CapSesne
*  CSD component.
*
* Note:
*
********************************************************************************
* Copyright 2014, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "CapSense_CSHL.h"
#include "CapSense_PVT.h"

/* SmartSense functions */
#if (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)
    extern void CapSense_UpdateThresholds(uint32 sensor);
#endif /* (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) */

/* Median filter function prototype */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_MEDIAN_FILTER)))
    uint16 CapSense_MedianFilter(uint16 x1, uint16 x2, uint16 x3);
#endif /* CapSense_RAW_FILTER_MASK && CapSense_POS_FILTERS_MASK */

/* Averaging filter function prototype */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_AVERAGING_FILTER)) )
    uint16 CapSense_AveragingFilter(uint16 x1, uint16 x2, uint16 x3);
#endif /* CapSense_RAW_FILTER_MASK && CapSense_POS_FILTERS_MASK */

/* IIR2Filter(1/2prev + 1/2cur) filter function prototype */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR2_FILTER)) )
    uint16 CapSense_IIR2Filter(uint16 x1, uint16 x2);
#endif /* CapSense_RAW_FILTER_MASK && CapSense_POS_FILTERS_MASK */

/* IIR4Filter(3/4prev + 1/4cur) filter function prototype */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR4_FILTER)) )
    uint16 CapSense_IIR4Filter(uint16 x1, uint16 x2);
#endif /* CapSense_RAW_FILTER_MASK && CapSense_POS_FILTERS_MASK */

/* IIR8Filter(7/8prev + 1/8cur) filter function prototype - RawCounts only */
#if (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER))
    uint16 CapSense_IIR8Filter(uint16 x1, uint16 x2);
#endif /* CapSense_RAW_FILTER_MASK */

/* IIR16Filter(15/16prev + 1/16cur) filter function prototype - RawCounts only */
#if (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER))
    uint16 CapSense_IIR16Filter(uint16 x1, uint16 x2);
#endif /* CapSense_RAW_FILTER_MASK */

/* JitterFilter filter function prototype */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_JITTER_FILTER)) )
    uint16 CapSense_JitterFilter(uint16 x1, uint16 x2);
#endif /* CapSense_RAW_FILTER_MASK && CapSense_POS_FILTERS_MASK */

/* Storage of filters data */
#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)) || \
      (0u != (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER)) )

    uint16 CapSense_rawFilterData1[CapSense_TOTAL_SENSOR_COUNT];
    uint16 CapSense_rawFilterData2[CapSense_TOTAL_SENSOR_COUNT];

#elif ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER))   || \
        (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER))   || \
        (0u != (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER)) || \
        (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER))   || \
        (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER)) )
        
    uint16 CapSense_rawFilterData1[CapSense_TOTAL_SENSOR_COUNT];
#endif  /* ( (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER) || \
        *    (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER) )
        */

extern uint16 CapSense_sensorRaw[CapSense_TOTAL_SENSOR_COUNT];
extern uint8 CapSense_sensorEnableMask[CapSense_TOTAL_SENSOR_MASK];
extern const uint8 CapSense_widgetNumber[CapSense_TOTAL_SENSOR_COUNT];

#if (CapSense_TUNING_METHOD != CapSense__TUNING_NONE)
	extern uint32 CapSense_widgetResolution[CapSense_RESOLUTIONS_TBL_SIZE];	
#endif /* (CapSense_TUNING_METHOD != CapSense__TUNING_NONE) */

uint16 CapSense_sensorBaseline[CapSense_TOTAL_SENSOR_COUNT] = {0u};
uint8 CapSense_sensorBaselineLow[CapSense_TOTAL_SENSOR_COUNT] = {0u};
uint8 CapSense_sensorSignal[CapSense_TOTAL_SENSOR_COUNT] = {0u};
uint8 CapSense_sensorOnMask[CapSense_TOTAL_SENSOR_MASK] = {0u};

uint8 CapSense_lowBaselineResetCnt[CapSense_TOTAL_SENSOR_COUNT];
uint8 CapSense_lowBaselineReset[CapSense_TOTAL_IMMUNITY_NUM] = {
    5u, 5u, 5u, 5u, 
};



#if (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)
	extern CapSense_CONFIG_TYPE_P4_v2_0 CapSense_config;
#endif  /* (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) */

/* Generated by Customizer */
const uint8 CapSense_fingerThreshold[] = {
    100u, 100u, 100u, 100u, 
};

const uint8 CapSense_noiseThreshold[] = {
    20u, 20u, 20u, 20u, 
};

const uint8 CapSense_hysteresis[] = {
    10u, 10u, 10u, 10u, 
};

const uint8 CapSense_debounce[] = {
    5u, 5u, 5u, 5u, 
};

static uint8 CapSense_debounceCounter[] = {
    0u, 0u, 0u, 0u, 0u, 
};

const uint8 CapSense_rawDataIndex[] = {
    0u, /* Button0__BTN */
    1u, /* Button1__BTN */
    2u, /* Button2__BTN */
    3u, /* Button3__BTN */
};

const uint8 CapSense_numberOfSensors[] = {
    1u, /* Button0__BTN */
    1u, /* Button1__BTN */
    1u, /* Button2__BTN */
    1u, /* Button3__BTN */
};




/*******************************************************************************
* Function Name: CapSense_BaseInit
********************************************************************************
*
* Summary:
*  Loads the CapSense_sensorBaseline[sensor] array element with an 
*  initial value which is equal to the raw count value. 
*  Resets to zero CapSense_sensorBaselineLow[sensor] and 
*  CapSense_sensorSignal[sensor] array element.
*  Loads the CapSense_debounceCounter[sensor] array element with the initial 
*  value equal CapSense_debounce[].
*  Loads the CapSense_rawFilterData2[sensor] and 
*  CapSense_rawFilterData2[sensor] array element with an 
*  initial value which is equal to the raw count value if the raw data filter is enabled.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  None
*
* Global Variables:
*  CapSense_sensorBaseline[]    - used to store the baseline value.
*  CapSense_sensorBaselineLow[] - used to store the fraction byte of 
*  the baseline value.
*  CapSense_sensorSignal[]      - used to store a difference between 
*  the current value of raw data and the previous value of the baseline.
*  CapSense_debounceCounter[]   - used to store the current debounce 
*  counter of the sensor. The widgets which have this parameter are buttons, matrix 
*  buttons, proximity, and guard. All other widgets don't have the  debounce parameter
*  and use the last element of this array with value 0 (it means no debounce).
*  CapSense_rawFilterData1[]    - used to store a previous sample of 
*  any enabled raw data filter.
*  CapSense_rawFilterData2[]    - used to store before a previous sample
*  of the enabled raw data filter. Required only for median or average filters.
*
* Side Effects:
*  None
* 
*******************************************************************************/
void CapSense_BaseInit(uint32 sensor)
{
    #if ((CapSense_TOTAL_BUTTONS_COUNT) || (CapSense_TOTAL_MATRIX_BUTTONS_COUNT) || \
         (CapSense_TOTAL_GENERICS_COUNT))
        uint8 widget = CapSense_widgetNumber[sensor];
    #endif /* ((CapSense_TOTAL_MATRIX_BUTTONS_COUNT) || (CapSense_TOTAL_MATRIX_BUTTONS_COUNT)) */
    
    #if (CapSense_TOTAL_MATRIX_BUTTONS_COUNT)
        uint8 debounceIndex;
    #endif  /* (CapSense_TOTAL_MATRIX_BUTTONS_COUNT) */
    
    #if (CapSense_TOTAL_GENERICS_COUNT)
        /* Exclude generic widget */
        if(widget < CapSense_END_OF_WIDGETS_INDEX)
        {
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */
    
    /* Initialize Baseline */
    CapSense_sensorBaseline[sensor] = CapSense_sensorRaw[sensor];
    CapSense_sensorBaselineLow[sensor] = 0u;
    CapSense_sensorSignal[sensor] = 0u;
        
    CapSense_debounceCounter[widget] =  CapSense_debounce[widget];

    
    #if ((0u != (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)) ||\
         (0u != (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER)))

        CapSense_rawFilterData1[sensor] = CapSense_sensorRaw[sensor];
        CapSense_rawFilterData2[sensor] = CapSense_sensorRaw[sensor];
    
    #elif ((CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER) ||\
           (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER) ||\
           (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER) ||\
           (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER) ||\
           (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER))
            
        CapSense_rawFilterData1[sensor] = CapSense_sensorRaw[sensor];
    
    #else
        /* No Raw filters */
    #endif  /* ((CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER) || \
            *   (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER))
            */
    
    #if (CapSense_TOTAL_GENERICS_COUNT)
        /* Exclude generic widget */
        }
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */
}


/*******************************************************************************
* Function Name: CapSense_InitializeSensorBaseline
********************************************************************************
*
* Summary:
*  Loads the CapSense_sensorBaseline[sensor] array element with an 
*  initial value by scanning the selected sensor (one channel design) or a pair 
*  of sensors (two channels designs). The raw count value is copied into the 
*  baseline array for each sensor. The raw data filters are initialized if 
*  enabled.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  None
*
* Global Variables:
*  None
*
* Side Effects:
*  None
* 
*******************************************************************************/
void CapSense_InitializeSensorBaseline(uint32 sensor)
{
    /* Scan sensor */
    CapSense_ScanSensor(sensor);
    while(CapSense_IsBusy() != 0u)
    {
        /* Wait while sensor is busy */
    }
    
    /* Initialize Baseline, Signal and debounce counters */       
    CapSense_BaseInit(sensor);
}


/*******************************************************************************
* Function Name: CapSense_InitializeAllBaselines
********************************************************************************
*
* Summary:
*  Uses the CapSense_InitializeSensorBaseline function to load the 
*  CapSense_sensorBaseline[] array with an initial values by scanning 
*  all the sensors. The raw count values are copied into the baseline array for 
*  all the sensors. The raw data filters are initialized if enabled.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global Variables:
*  None
*
* Side Effects:
*  None
* 
*******************************************************************************/
void CapSense_InitializeAllBaselines(void)
{
    uint32 i;
    
	for(i = 0u; i < CapSense_TOTAL_SCANSLOT_COUNT; i++)
	{
    	CapSense_InitializeSensorBaseline(i);
	}
}


/*******************************************************************************
* Function Name: CapSense_InitializeEnabledBaselines
********************************************************************************
*
* Summary:
*  Scans all the enabled widgets and the raw count values are copied into the 
*  baseline array for all the sensors enabled in the scanning process. The baselines 
*  are initialized with zero values for sensors disabled from the scanning process. 
*  The raw data filters are initialized if enabled.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global Variables:
*  CapSense_sensorRaw[] - used to store the sensors raw data.
*  CapSense_sensorEnableMask[ ] - used to store bit masks of the enabled sensors.
*
* Side Effects:
*  None
* 
*******************************************************************************/
void CapSense_InitializeEnabledBaselines(void)
{
    uint32 i;
    uint32 isSensorEnabled;
    
    CapSense_ScanEnabledWidgets();
    while(CapSense_IsBusy() != 0u)
    {
        /* Wait while sensor is busy */
    }
    
    for(i = 0u; i < CapSense_TOTAL_SENSOR_COUNT; i++)
    {
        isSensorEnabled = CapSense_GetBitValue(CapSense_sensorEnableMask, i);
        
        /* Clear raw data if sensor is disabled from scanning process */
        if(isSensorEnabled != 0u)
        {
            /* Initialize baselines */
            CapSense_BaseInit(i);
        }
    }
}  


/*******************************************************************************
* Function Name: CapSense_UpdateBaselineNoThreshold
********************************************************************************
*
* Summary:
*  Updates the CapSense_sensorBaseline[sensor] array element using the 
*  LP filter with k = 256. The signal calculates the difference of count by 
*  subtracting the previous baseline from the current raw count value and stores
*  it in CapSense_sensorSignal[sensor]. 
*  If the auto reset option is enabled, the baseline updated regards the noise threshold. 
*  If the auto reset option is disabled, the baseline stops updating; baseline is loaded 
*  with a raw count value if a signal is greater than zero and if signal is less 
*  than noise threshold.
*  Raw data filters are applied to the values if enabled before baseline 
*  calculation.
*
*  This API does not update the thresholds in the Smartsense Mode.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  None
*
* Global Variables:
*  CapSense_widgetNumber[]  - stores widget numbers.
*  CapSense_sensorBaseline[]    - used to store baseline value.
*  CapSense_sensorBaselineLow[] - used to store fraction byte of 
*  baseline value.
*  CapSense_sensorSignal[]      - used to store difference between 
*  current value of raw data and previous value of baseline.
*  CapSense_rawFilterData1[]    - used to store previous sample of 
*  any enabled raw data filter.
*  CapSense_rawFilterData2[]    - used to store before previous sample
*  of enabled raw data filter. Only required for median or average filters.
* 
* Side Effects:
*  None
* 
*******************************************************************************/
void CapSense_UpdateBaselineNoThreshold(uint32 sensor)
{
    uint32 calc;
	uint32 sign;
    uint16 tempRaw;
    uint16 filteredRawData;
    uint8 widget = CapSense_widgetNumber[sensor];
    uint8 noiseThreshold = CapSense_noiseThreshold[widget];
    
    #if (CapSense_TOTAL_GENERICS_COUNT)
        /* Exclude generic widget */
        if(widget < CapSense_END_OF_WIDGETS_INDEX)
        {
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */
    
    filteredRawData = CapSense_sensorRaw[sensor];
    
    #if (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)
        tempRaw = filteredRawData;
        filteredRawData = CapSense_MedianFilter(filteredRawData, CapSense_rawFilterData1[sensor], 
                                                        CapSense_rawFilterData2[sensor]);
        CapSense_rawFilterData2[sensor] = CapSense_rawFilterData1[sensor];
        CapSense_rawFilterData1[sensor] = tempRaw;
        
    #elif (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER)
        tempRaw = filteredRawData;
        filteredRawData = CapSense_AveragingFilter(filteredRawData, CapSense_rawFilterData1[sensor],
                                                           CapSense_rawFilterData2[sensor]);
        CapSense_rawFilterData2[sensor] = CapSense_rawFilterData1[sensor];
        CapSense_rawFilterData1[sensor] = tempRaw;
        
    #elif (CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER)
        filteredRawData = CapSense_IIR2Filter(filteredRawData, CapSense_rawFilterData1[sensor]);
        CapSense_rawFilterData1[sensor] = filteredRawData;
        
    #elif (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER)
        filteredRawData = CapSense_IIR4Filter(filteredRawData, CapSense_rawFilterData1[sensor]);
        CapSense_rawFilterData1[sensor] = filteredRawData;
            
    #elif (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER)
        filteredRawData = CapSense_JitterFilter(filteredRawData, CapSense_rawFilterData1[sensor]);
        CapSense_rawFilterData1[sensor] = filteredRawData;
        
    #elif (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER)
        filteredRawData = CapSense_IIR8Filter(filteredRawData, CapSense_rawFilterData1[sensor]);
        CapSense_rawFilterData1[sensor] = filteredRawData;
        
    #elif (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER)
        filteredRawData = CapSense_IIR16Filter(filteredRawData, CapSense_rawFilterData1[sensor]);
        CapSense_rawFilterData1[sensor] = filteredRawData;
        
    #else
        /* No Raw filters */
    #endif  /* (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER) */

	#if (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)
	    MeasureNoiseEnvelope_P4_v2_0(&CapSense_config, (uint8)sensor, CapSense_sensorRaw);
	#endif /* (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) */

    /* Baseline calculation */
    /* Calculate difference RawData[cur] - Baseline[prev] */
    if(filteredRawData >= CapSense_sensorBaseline[sensor])
    {
		CapSense_lowBaselineResetCnt[sensor] = 0u;	
        tempRaw = filteredRawData - CapSense_sensorBaseline[sensor];
        sign = 1u;    /* Positive difference - Calculate the Signal */
    }
    else
    {
        tempRaw = CapSense_sensorBaseline[sensor] - filteredRawData;
        sign = 0u;    /* Negative difference - Do NOT calculate the Signal */
    }

	#if (CapSense_TUNING_METHOD != CapSense__TUNING_NONE)
	if((sign == 0u) && (tempRaw > (uint16) CapSense_negativeNoiseThreshold[widget]))
	#else
	if((sign == 0u) && (tempRaw > (uint16) CapSense_NEGATIVE_NOISE_THRESHOLD))
	#endif /* (CapSense_TUNING_METHOD != CapSense__TUNING_NONE) */ 
    {
        if(CapSense_lowBaselineResetCnt[sensor] >= CapSense_LOW_BASELINE_RESET)
        {
            CapSense_BaseInit(sensor);
            CapSense_lowBaselineResetCnt[sensor] = 0u;
        }
        else
        {
            CapSense_lowBaselineResetCnt[sensor]++;
        }
    }
    else
    {
        #if (CapSense_AUTO_RESET == CapSense_AUTO_RESET_DISABLE)
		#if (CapSense_TUNING_METHOD != CapSense__TUNING_NONE)			
			/* Update Baseline if lower than noiseThreshold */
			if ( (tempRaw <= noiseThreshold) || 
			 ((tempRaw < (uint16) CapSense_negativeNoiseThreshold[widget])
			   && (sign == 0u)))
			{
		#else
			/* Update Baseline if lower than noiseThreshold */
			if ( (tempRaw <= noiseThreshold) || 
				 ((tempRaw < (uint16) CapSense_NEGATIVE_NOISE_THRESHOLD)
				   && (sign == 0u)))
			{
		#endif /* (CapSense_TUNING_METHOD != CapSense__TUNING_NONE) */ 
        #endif /* (CapSense_AUTO_RESET == CapSense_AUTO_RESET_DISABLE) */
                /* Make full Baseline 23 bits */
                calc = (uint32) CapSense_sensorBaseline[sensor] << 8u;
                calc |= (uint32) CapSense_sensorBaselineLow[sensor];

                /* Add Raw Data to Baseline */
                calc += filteredRawData;

                /* Sub the high Baseline */
                calc -= CapSense_sensorBaseline[sensor];

                /* Put Baseline and BaselineLow */
                CapSense_sensorBaseline[sensor] = ((uint16) (calc >> 8u));
                CapSense_sensorBaselineLow[sensor] = ((uint8) calc);

                CapSense_lowBaselineResetCnt[sensor] = 0u;
        #if (CapSense_AUTO_RESET == CapSense_AUTO_RESET_DISABLE)
            }
        #endif /* (CapSense_AUTO_RESET == CapSense_AUTO_RESET_DISABLE) */
    }

    /* Calculate Signal if positive difference > noiseThreshold */
    if((tempRaw > (uint16) noiseThreshold) && (sign != 0u))
    {
        #if (CapSense_SIGNAL_SIZE == CapSense_SIGNAL_SIZE_UINT8)
            /* Over flow defence for uint8 */
            if (tempRaw > 0xFFu)
            {
                CapSense_sensorSignal[sensor] = 0xFFu;
            }    
            else 
            {
                CapSense_sensorSignal[sensor] = ((uint8) tempRaw);
            }
        #else
            CapSense_sensorSignal[sensor] = ((uint16) tempRaw);
        #endif  /* (CapSense_SIGNAL_SIZE == CapSense_SIGNAL_SIZE_UINT8) */
    }
    else
    {
        /* Signal is zero */
        CapSense_sensorSignal[sensor] = 0u;
    }

    #if (CapSense_TOTAL_GENERICS_COUNT)
        /* Exclude generic widget */
        }
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */
}


/*******************************************************************************
* Function Name: CapSense_UpdateSensorBaseline
********************************************************************************
*
* Summary:
*  Updates the CapSense_sensorBaseline[sensor] array element using the 
*  LP filter with k = 256. The signal calculates the difference of count by 
*  subtracting the previous baseline from the current raw count value and stores
*  it in CapSense_sensorSignal[sensor]. 
*  If the auto reset option is enabled, the baseline updated regards the noise threshold. 
*  If the auto reset option is disabled, the baseline stops updating. 
*  Baseline is loaded with raw count value if a signal is greater than zero and  
*  if signal is less than noise threshold.
*  Raw data filters are applied to the values if enabled before baseline 
*  calculation.
*  This API updates the thresholds in the Smartsense Mode.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  None
*
* Global Variables:
*  CapSense_widgetNumber[]  - stores widget numbers.
* 
* Side Effects:
*  None
* 
*******************************************************************************/
 void CapSense_UpdateSensorBaseline(uint32 sensor)
{
    #if (CapSense_TOTAL_GENERICS_COUNT)
		uint32 widget;
		
		widget = CapSense_widgetNumber[sensor];
	
        /* Exclude generic widget */
        if(widget < CapSense_END_OF_WIDGETS_INDEX)
        {
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */

    #if (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)
    	CapSense_UpdateThresholds(sensor);
    #endif /* (CapSense_TUNING_METHOD == CapSense__TUNING_AUTO) */

    CapSense_UpdateBaselineNoThreshold(sensor);
    
    #if (CapSense_TOTAL_GENERICS_COUNT)
        /* Exclude generic widget */
        }
    #endif  /* CapSense_TOTAL_GENERICS_COUNT */
}


/*******************************************************************************
* Function Name: CapSense_UpdateEnabledBaselines
********************************************************************************
*
* Summary:
*  Checks CapSense_sensorEnableMask[] array and calls the 
*  CapSense_UpdateSensorBaseline function to update the baselines 
*  for the enabled sensors.
*
* Parameters:
*  None
*
* Return:
*  None
*
* Global Variables:
*  CapSense_sensorEnableMask[] - used to store the sensor scanning 
*  state.
*  CapSense_sensorEnableMask[0] contains the masked bits for sensors 
*   0 through 7 (sensor 0 is bit 0, sensor 1 is bit 1).
*  CapSense_sensorEnableMask[1] contains the masked bits for 
*  sensors 8 through 15 (if needed), and so on.
*  0 - sensor doesn't scan by CapSense_ScanEnabledWidgets().
*  1 - sensor scans by CapSense_ScanEnabledWidgets().
* 
* Side Effects:
*  None
* 
*******************************************************************************/
 void CapSense_UpdateEnabledBaselines(void)
{
    uint32 i;
    uint32 isSensorEnabled;
    
    for(i = 0u; i < CapSense_TOTAL_SENSOR_COUNT; i++)
    {
        isSensorEnabled = CapSense_GetBitValue(CapSense_sensorEnableMask, i);
        
        if(0u != isSensorEnabled)
        {
            CapSense_UpdateSensorBaseline(i);
        }
    }
}


/*******************************************************************************
* Function Name: CapSense_GetBaselineData
********************************************************************************
*
* Summary:
*  This function reads the sensor baseline from the component.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  This API returns the baseline value of the sensor indicated by an argument.
*
* Global Variables:
*  CapSense_sensorBaseline[] - the array with an initial values by scanning
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint16 CapSense_GetBaselineData(uint32 sensor)
{
	return (CapSense_sensorBaseline[sensor]);
}


/*******************************************************************************
* Function Name: CapSense_SetBaselineData
********************************************************************************
*
* Summary:
*  This API writes the data value passed as an argument to the sensor baseline array.
*
* Parameters:
*  sensor:  Sensor number.
*  data:    Sensor baseline.
*
* Return:
*  None.
*
* Global Variables:
*  CapSense_sensorBaseline[] - the array with initial values by scanning
* 
* Side Effects:
*  None
*
*******************************************************************************/
void CapSense_SetBaselineData(uint32 sensor, uint16 data)
{
	CapSense_sensorBaseline[sensor] = data;
}


/*******************************************************************************
* Function Name: CapSense_GetDiffCountData
********************************************************************************
*
* Summary:
*  This function returns the Sensor Signal from the component.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  This API returns the difference count value of the sensor indicated by the 
*  argument. 
*
* Global Variables:
*  CapSense_sensorSignal[] - the array with difference counts for sensors
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetDiffCountData(uint32 sensor)
{
	return (CapSense_sensorSignal[sensor]);
}


/*******************************************************************************
* Function Name: CapSense_SetDiffCountData
********************************************************************************
*
* Summary:
*  This API writes the data value passed as an argument to the sensor signal array.
*
* Parameters:
*  sensor:  Sensor number.
*  value:   Sensor signal.
*
* Return:
*  None. 
*
* Global Variables:
*  CapSense_sensorSignal[] - the array with difference counts for sensors
* 
* Side Effects:
*  None
*
*******************************************************************************/
void CapSense_SetDiffCountData(uint32 sensor, uint8 value)
{
    CapSense_sensorSignal[sensor] = value;
}


/*******************************************************************************
* Function Name: CapSense_GetFingerThreshold
********************************************************************************
*
* Summary:
*  This function reads the finger threshold from the component.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  This API returns the finger threshold of the widget indicated by the argument. 
*
* Global Variables:
*  CapSense_fingerThreshold[] - This array contains the level of signal 
*  for each widget that is determined if a finger is present on the widget.
*
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetFingerThreshold(uint32 widget)
{
	return(CapSense_fingerThreshold[widget]);
}


/*******************************************************************************
* Function Name: CapSense_GetNoiseThreshold
********************************************************************************
*
* Summary:
*  This function reads the noise threshold from the component.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  This API returns the noise threshold of the widget indicated by the argument. 
*
* Global Variables:
*  CapSense_noiseThreshold[] - This array contains the level of signal 
*  for each widget that determines the level of noise in the capacitive scan.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetNoiseThreshold(uint32 widget)
{
	return (CapSense_noiseThreshold[widget]);
}


/*******************************************************************************
* Function Name: CapSense_GetFingerHysteresis
********************************************************************************
*
* Summary:
*  This function reads the Hysteresis value from the component.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  This API returns the Hysteresis of the widget indicated by the argument. 
*
* Global Variables:
*  CapSense_hysteresis[] - This array contains the hysteresis value 
*   for each widget.
*  CapSense_widgetNumberSld - structure with dynamic sliders' parameters.
*
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetFingerHysteresis(uint32 widget)
{
	return(CapSense_hysteresis[widget]);
}


/*******************************************************************************
* Function Name: CapSense_GetNegativeNoiseThreshold
********************************************************************************
*
* Summary:
*  This function reads the negative noise threshold from the component.
*
* Parameters: 
*  None.
*
* Return Value: 
*  This API returns the negative noise threshold
*
* Global Variables:
*  CapSense_negativeNoiseThreshold - This variable specifies the negative 
*   difference between the raw count and baseline levels for Baseline resetting
*   to the raw count level.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetNegativeNoiseThreshold(uint32 widget)
{
#if (CapSense_TUNING_METHOD != CapSense__TUNING_NONE)
	return(CapSense_negativeNoiseThreshold[widget]);
#else
	return(CapSense_NEGATIVE_NOISE_THRESHOLD);
#endif /* (CapSense_TUNING_METHOD != CapSense__TUNING_NONE) */ 
}


#if(CapSense_TUNING_METHOD != CapSense__TUNING_NONE)
	/*******************************************************************************
	* Function Name: CapSense_SetFingerThreshold
	********************************************************************************
	*
	* Summary:
	*  This API sets the finger threshold value for a widget. 
	*
	* Parameters: 
	*  sensorNumber:  widget index. 
	*  value:  Finger threshold value for the widget.
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_fingerThreshold[] - This array contains the level of signal 
	*   for each widget that determines if a finger is present on the widget.
	* 
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetFingerThreshold(uint32 widget, uint8 value)
	{	
		CapSense_fingerThreshold[widget] = value;
	}


	/*******************************************************************************
	* Function Name: CapSense_SetNoiseThreshold
	********************************************************************************
	*
	* Summary:
	*  This API sets the Noise Threshold value for each widget.
	*
	* Parameters:
	*  widget:  Sensor index number.
	*  value:   Noise Threshold value for widget.
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_noiseThreshold[] - This array contains the level of signal 
	*   for each widget that determines the level of noise in the capacitive scan.
	* 
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetNoiseThreshold(uint32 widget, uint8 value)
	{
		CapSense_noiseThreshold[widget] = value;
	}


	/*******************************************************************************
	* Function Name: CapSense_SetFingerHysteresis
	********************************************************************************
	*
	* Summary:
	*  This API sets the Hysteresis value of a widget
	*
	* Parameters: 
	*  value:  Hysteresis value for widgets.
	*  widget:  widget number
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_hysteresis[] - This array contains the hysteresis value for each widget.
	*  CapSense_widgetNumberSld - structure with dynamic sliders' parameters.
	*
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetFingerHysteresis(uint32 widget, uint8 value)
	{
		CapSense_hysteresis[widget] = value;
	}


	/*******************************************************************************
	* Function Name: CapSense_SetNegativeNoiseThreshold
	********************************************************************************
	*
	* Summary:
	*  This API sets the Negative Noise Threshold value of a widget
	*
	* Parameters: 
	*  value:  Negative Noise Threshold value for widgets.
	*  widget: widget number
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_negativeNoiseThreshold  - This parameter specifies the negative 
	*   difference between the raw count and baseline levels for Baseline resetting to 
	*   the raw count level.
	* 
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetNegativeNoiseThreshold(uint32 widget, uint8 value)
	{
		CapSense_negativeNoiseThreshold[widget] = value;
	}


	/*******************************************************************************
	* Function Name: CapSense_SetDebounce
	********************************************************************************
	*
	* Summary:
	*  This API sets the debounce value for a widget.
	*
	* Parameters: 
	*  value:  Debounce value for widget.
	*  widget: widget index.
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_debounce[] - This array contains the debounce value for each widget.
	*  CapSense_widgetNumberSld - structure with dynamic sliders' parameters.
	*
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetDebounce(uint32 widget, uint8 value)
	{
		CapSense_debounce[widget] = value;
	}


	/*******************************************************************************
	* Function Name: CapSense_SetLowBaselineReset
	********************************************************************************
	*
	* Summary:
	*  This API sets the low baseline reset threshold value for the sensor.
	*
	* Parameters: 
	*  value: low baseline reset threshold value.
	*  sensor: Sensor index.
	*
	* Return Value: 
	*  None
	*
	* Global Variables:
	*  CapSense_lowBaselineReset[] - This array contains the Baseline update 
	*  threshold value for each sensor.
	* 
	* Side Effects:
	*  None
	*
	*******************************************************************************/
	void CapSense_SetLowBaselineReset(uint32 sensor, uint8 value)
	{
		CapSense_lowBaselineReset[sensor] = value;
	}
#endif /* (CapSense_TUNING_METHOD != CapSense__TUNING_NONE) */


/*******************************************************************************
* Function Name: CapSense_CheckIsSensorActive
********************************************************************************
*
* Summary:
*  Compares the CapSense_sensorSignal[sensor] array element to the finger
*  threshold of the widget it belongs to. The hysteresis and debounce are taken into 
*  account. The hysteresis is added or subtracted from the finger threshold 
*  based on whether the sensor is currently active. 
*  If the sensor is active, the threshold is lowered by the hysteresis amount.
*  If the sensor is inactive, the threshold is raised by the hysteresis amount.
*  The debounce counter is added to the sensor active transition.
*  This function updates the CapSense_sensorOnMask[] array element.
*
* Parameters:
*  sensor:  Sensor number.
*
* Return:
*  Returns sensor state 1 if active, 0 if not active.
*
* Global Variables:
*  CapSense_sensorSignal[] - used to store the difference between 
*  the current value of raw data and a previous value of the baseline.
*  CapSense_debounceCounter[]   - used to store the current debounce 
*  counter of the sensor. the widget which have this parameter are buttons, matrix 
*  buttons, proximity, and guard. All other widgets don't have the  debounce parameter
*  and use the last element of this array with value 0 (it means no debounce).
*  CapSense_sensorOnMask[] - used to store the sensors on/off state.
*  CapSense_sensorOnMask[0] contains the masked bits for sensors 
*   0 through 7 (sensor 0 is bit 0, sensor 1 is bit 1).
*  CapSense_sensorEnableMask[1] contains the masked bits for 
*  sensors 8 through 15 (if needed), and so on.
*  0 - sensor is inactive.
*  1 - sensor is active.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint32 CapSense_CheckIsSensorActive(uint32 sensor)
{	
	uint8 widget;
	uint8 debounce;
	uint8 debounceIndex;
    
	uint8 fingerThreshold;
	uint8 hysteresis;
	
    /* Prepare to find debounce counter index */
    widget = CapSense_widgetNumber[sensor];
	
    fingerThreshold = CapSense_fingerThreshold[widget];
	hysteresis = CapSense_hysteresis[widget];
	debounce = CapSense_debounce[widget];	
	
	    debounceIndex = widget;

	
    /* Was on */
    if (0u != CapSense_GetBitValue(CapSense_sensorOnMask, sensor))
    {
        /* Hysteresis minus */
        if (CapSense_sensorSignal[sensor] < (fingerThreshold - hysteresis))
        {
	   		CapSense_SetBitValue(CapSense_sensorOnMask, sensor, 0u);
			/* Sensor inactive - reset Debounce counter */
            CapSense_debounceCounter[debounceIndex] = debounce;
        }
    }
    else    /* Was off */
    {
        /* Hysteresis plus */
        if (CapSense_sensorSignal[sensor] >= (fingerThreshold + hysteresis))
        {
			CapSense_debounceCounter[debounceIndex]--;
            /* Sensor active, decrement debounce counter */
            if(CapSense_debounceCounter[debounceIndex] == 0u)
            {
                CapSense_SetBitValue(CapSense_sensorOnMask, sensor, 1u); 
            }
        }
        else
        {
            /* Sensor inactive - reset Debounce counter */
            CapSense_debounceCounter[debounceIndex] = debounce;
        }
    }
    
    return ((uint32)((0u != CapSense_GetBitValue(CapSense_sensorOnMask, sensor)) ? 1u : 0u));
}


/*******************************************************************************
* Function Name: CapSense_CheckIsWidgetActive
********************************************************************************
*
* Summary:
*  Compares the CapSense_sensorSignal[] array element to the finger threshold of 
* the widget it belongs to. The hysteresis and debounce are taken into account. 
* The hysteresis is added or subtracted from the finger threshold based on whether
*  the sensor is currently active. If the sensor is active, the threshold is 
*  lowered by the hysteresis amount. If the sensor is inactive, the threshold 
*  is raised by the hysteresis amount. 
* The debounce counter added to the sensor active transition. 
* This function updates CapSense_sensorOnMask[] array element
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  Returns widget sensor state 1 if one or more sensors within widget is/are 
*  active, 0 if all sensors within widget are inactive.
*
* Global Variables:
*  rawDataIndex[] � Contains the  1st sensor position in the widget
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint32 CapSense_CheckIsWidgetActive(uint32 widget)
{
    uint32 rawIndex = (uint32)CapSense_rawDataIndex[widget];
    uint32 numOfSensors = (uint32)CapSense_numberOfSensors[widget] + rawIndex;
    uint32 state = 0u;

    /* Check all sensors of widget */
    do
    {
        if(CapSense_CheckIsSensorActive((uint32)rawIndex) != 0u)
        {
            state = CapSense_SENSOR_IS_ACTIVE;
        }
        rawIndex++;
    }
    while(rawIndex < numOfSensors);
    

    
    return state;
}


/*******************************************************************************
* Function Name: CapSense_CheckIsAnyWidgetActive
********************************************************************************
*
* Summary:
*  Compares all the sensors of the CapSense_Signal[] array to their finger 
*  threshold. Calls CapSense_CheckIsWidgetActive() for each widget so 
*  the CapSense_sensorOnMask[] array is up to date after calling this 
*  function.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  Returns 1 if any widget is active, 0 none of widgets are active.
*
* Global Variables:
*  None
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint32 CapSense_CheckIsAnyWidgetActive(void)
{

	    uint32 i;
    uint32 state = 0u;
    
    for(i = 0u; i < CapSense_TOTAL_WIDGET_COUNT; i++)
    {
        if (CapSense_CheckIsWidgetActive(i) != 0u)
        {
            state = CapSense_WIDGET_IS_ACTIVE;
        }
    }
    


    return state;
}


/*******************************************************************************
* Function Name: CapSense_EnableWidget
********************************************************************************
*
* Summary:
*  Enables all the widget elements (sensors) to the scanning process.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  None
*
* Global Variables:
*  CapSense_sensorEnableMask[] - used to store the sensor scanning 
*  state.
*  CapSense_sensorEnableMask[0] contains the masked bits for sensors 
*  0 through 7 (sensor 0 is bit 0, sensor 1 is bit 1).
*  CapSense_sensorEnableMask[1] contains the masked bits for 
*  sensors 8 through 15 (if needed), and so on.
*  0 - sensor doesn't scan by CapSense_ScanEnabledWidgets().
*  1 - sensor scans by CapSense_ScanEnabledWidgets().
* 
* Side Effects:
*  None
*
*******************************************************************************/
void CapSense_EnableWidget(uint32 widget)
{

    uint8 rawIndex = CapSense_rawDataIndex[widget];
    uint8 numOfSensors = CapSense_numberOfSensors[widget] + rawIndex;
    
    /* Enable all sensors of widget */
    do
    {
        CapSense_SetBitValue(CapSense_sensorEnableMask, (uint32)rawIndex, 1u);
        rawIndex++;
    }
    while(rawIndex < numOfSensors);
}


/*******************************************************************************
* Function Name: CapSense_DisableWidget
********************************************************************************
*
* Summary:
*  Disables all the widget elements (sensors) from the scanning process.
*
* Parameters:
*  widget:  widget number.
*
* Return:
*  None
*
* Global Variables:
*  CapSense_sensorEnableMask[] - used to store the sensor scanning 
*  state.
*  CapSense_sensorEnableMask[0] contains the masked bits for sensors 
*  0 through 7 (sensor 0 is bit 0, sensor 1 is bit 1).
*  CapSense_sensorEnableMask[1] contains the masked bits for 
*  sensors 8 through 15 (if needed), and so on.
*  0 - sensor isn't scanned by CapSense_ScanEnabledWidgets().
*  1 - sensor is scanned by CapSense_ScanEnabledWidgets().
* 
* Side Effects:
*  None
*
*******************************************************************************/
void CapSense_DisableWidget(uint32 widget)
{
    uint8 rawIndex = CapSense_rawDataIndex[widget];
    uint8 numOfSensors = CapSense_numberOfSensors[widget] + rawIndex;
   
    /* Disable all sensors of widget */
    do
    {

        CapSense_SetBitValue(CapSense_sensorEnableMask, (uint32)rawIndex, 0u);
        rawIndex++;
    }
    while(rawIndex < numOfSensors);
}


#if(CapSense_TOTAL_CENTROIDS_COUNT)
    /*******************************************************************************
    * Function Name: CapSense_FindMaximum
    ********************************************************************************
    *
    * Summary:
    *  Finds the index of the maximum element within a defined centroid. Checks 
    *  CapSense_sensorSignal[] within a defined centroid and 
    *  returns the index of the maximum element. The values below the finger threshold are 
    *  ignored.
    *  The centroid is defined by offset of the first element and a number of elements - count.
    *  The diplexed centroid requires at least two consecutive elements above
    *  FingerThreshold to find the index of the maximum element.
    * 
    * Parameters:
    *  offset:  Start index of centroid in CapSense_sensorSignal[] array.
    *  count:   number of elements within centroid.
    *  fingerThreshold:  Finger threshold.
    *  diplex:   pointer to diplex table.
    * 
    * Return:
    *  Returns the index of the maximum element within a defined centroid.
    *  If the index of the maximum element isn't found it returns 0xFF.
	*
	* Global Variables:
	*  CapSense_startOfSlider[] - contains the index of the first slider element
	* 
	* Side Effects: 
	*  None
	* 
    *******************************************************************************/
	#if (CapSense_IS_DIPLEX_SLIDER)
		uint8 CapSense_FindMaximum(uint8 offset, uint8 count, uint8 fingerThreshold, const uint8 *diplex)
	#else 
		uint8 CapSense_FindMaximum(uint8 offset, uint8 count, uint8 fingerThreshold)
	#endif /* (CapSense_IS_DIPLEX_SLIDER) */
    {
        uint8 i;
        #if (CapSense_IS_DIPLEX_SLIDER)        
            uint8 curPos = 0u;
            /* No centroid at Start */
            uint8 curCntrdSize = 0u;
            uint8 curCtrdStartPos = CapSense_MAXIMUM_CENTROID;
            /* Biggest centroid is zero */
            uint8 biggestCtrdSize = 0u;
            uint8 biggestCtrdStartPos = 0u;
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */
        uint8 maximum = CapSense_MAXIMUM_CENTROID;
		
        uint8 temp = 0u;
        uint8 *startOfSlider = &CapSense_sensorSignal[offset];

        #if (CapSense_IS_DIPLEX_SLIDER)        
            if(diplex != 0u)
            {
                /* Initialize */
                i = 0u;
                
                /* Make slider x2 as Diplexed */
                count <<= 1u;
                while(1u)
                { 
                    if (startOfSlider[curPos] > 0u)    /* Looking for centroids */
                    {
                        if (curCtrdStartPos == CapSense_MAXIMUM_CENTROID)
                        {
                            /* Start of centroid */
                            curCtrdStartPos = i;
                            curCntrdSize++;
                        }
                        else
                        {
                            curCntrdSize++;
                        }
                    }
                    else   /* Select biggest and indicate zero start */
                    {          
                        if(curCntrdSize > 0u)
                        {
                            /* We are at the end of current */
                            if(curCntrdSize > biggestCtrdSize)
                            {
                                biggestCtrdSize = curCntrdSize;
                                biggestCtrdStartPos = curCtrdStartPos;
                            }
                            
                            curCntrdSize = 0u;
                            curCtrdStartPos = CapSense_MAXIMUM_CENTROID;
                        }
                    }
                    
                    i++; 
                    curPos = diplex[i];
                    if(i == count)
                    {
                        break;
                    }            
                }
                    
                    /* Find biggest centroid if two are same size, last one wins
                       We are at the end of current */
                if (curCntrdSize >= biggestCtrdSize) 
                {
                    biggestCtrdSize = curCntrdSize;
                    biggestCtrdStartPos = curCtrdStartPos;
                }
            }
            else
            {
                /* Without diplexing */ 
                biggestCtrdSize = count;
            }
                        

            /* Check centroid size */
            #if (CapSense_IS_NON_DIPLEX_SLIDER)
                if((biggestCtrdSize >= 2u) || ((biggestCtrdSize == 1u) && (diplex == 0u)))
            #else                    
                if(biggestCtrdSize >= 2u)
            #endif /* (CapSense_IS_NON_DIPLEX_SLIDER) */
                {
                    for (i = biggestCtrdStartPos; i < (biggestCtrdStartPos + biggestCtrdSize); i++)
                    {
                        #if (CapSense_IS_DIPLEX_SLIDER && CapSense_IS_NON_DIPLEX_SLIDER)
                            if (diplex == 0u)
                            {
                                curPos = i;
                            }
                            else
                            {
                                curPos = diplex[i];
                            }                    
                        #elif (CapSense_IS_DIPLEX_SLIDER)                    
                            curPos = diplex[i];                    
                        #endif /* (CapSense_IS_DIPLEX_SLIDER && CapSense_IS_NON_DIPLEX_SLIDER) */
                        /* Looking for greater element within centroid */
                        if(startOfSlider[curPos] > fingerThreshold)
                        {
                            if(startOfSlider[curPos] > temp)
                            {
                                maximum = i;
                                temp = startOfSlider[curPos];
                            }
                        }
                    }
                } 
        #else
            for (i = 0u; i < count; i++)
            {                      
                /* Looking for greater element within centroid */
                if(startOfSlider[i] > fingerThreshold)
                {
                    if(startOfSlider[i] > temp)
                    {
                        maximum = i;
                        temp = startOfSlider[i];
                    }
                }
            }    
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */
        return (maximum);
    }
    
    
    /*******************************************************************************
    * Function Name: CapSense_CalcCentroid
    ********************************************************************************
    *
    * Summary:
    *  Returns a position value calculated according to the index of the maximum element and API
    *  resolution.
    *
    * Parameters:
    *  type:  widget type.
    *  diplex:  pointer to diplex table.
    *  maximum:  Index of maximum element within centroid.
    *  offset:   Start index of centroid in CapSense_sensorSignal[] array.
    *  count:    Number of elements within centroid.
    *  resolution:  multiplicator calculated according to centroid type and
    *  API resolution.
    *  noiseThreshold:  Noise threshold.
    * 
    * Return:
    *  Returns a position value of the slider.
	*
	* Side Effects: 
	*  None
	* 
    *******************************************************************************/
    uint8 CapSense_CalcCentroid(uint8 maximum, uint8 offset, 
                                        uint8 count, uint16 resolution, uint8 noiseThreshold)
    {
        #if ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_TOUCH_PADS_COUNT > 0u))
            uint8 posPrev;
            uint8 posNext;
        #endif /* ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT>0u) || (CapSense_TOTAL_TOUCH_PADS_COUNT>0u)) */
        
        /* Helps during centroid calculation */
        #if (CapSense_TOTAL_CENTROIDS_COUNT)
		    static uint8 CapSense_centroid[3u];
        #endif  /* (CapSense_TOTAL_CENTROIDS_COUNT) */
               
        #if (CapSense_IS_DIPLEX_SLIDER)                
            uint8 pos;
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */

        uint8 position;
        uint32 numerator;
        int32 denominator;
		
		uint8  *startOfSlider = &CapSense_sensorSignal[offset];
		
        #if ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_TOUCH_PADS_COUNT > 0u))
            posPrev = 0u;
            posNext = 0u;
        #endif /* ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT>0u) || (CapSense_TOTAL_TOUCH_PADS_COUNT>0u)) */
     
        #if (CapSense_ADD_SLIDER_TYPE)
            if(type == CapSense_TYPE_RADIAL_SLIDER)
            {
        #endif /* (CapSense_ADD_SLIDER_TYPE) */

            #if (CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u)                
                /* Copy Signal to the centroid array */
                CapSense_centroid[CapSense_POS] = startOfSlider[maximum];
                 
                /* Check borders for ROTARY Slider */
                if (maximum == 0u)                   /* Start of centroid */
                { 
                    CapSense_centroid[CapSense_POS_PREV] = startOfSlider[count - 1u];
                    CapSense_centroid[CapSense_POS_NEXT] = startOfSlider[1u];
                }
                else if (maximum == (count - 1u))    /* End of centroid */
                {
                    CapSense_centroid[CapSense_POS_PREV] = startOfSlider[maximum - 1u];
                    CapSense_centroid[CapSense_POS_NEXT] = startOfSlider[0u];
                }
                else                                /* Not first Not last */
                {
                    CapSense_centroid[CapSense_POS_PREV] = startOfSlider[maximum - 1u];
                    CapSense_centroid[CapSense_POS_NEXT] = startOfSlider[maximum + 1u];
                }
            #endif /* (CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) */

        #if (CapSense_ADD_SLIDER_TYPE)
            }
            else
            {
        #endif /* (CapSense_ADD_SLIDER_TYPE) */

            #if ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_TOUCH_PADS_COUNT > 0u))
                #if (CapSense_IS_DIPLEX_SLIDER && CapSense_IS_NON_DIPLEX_SLIDER)                    
                    /* Calculate next and previous near to maximum */
                    if(diplex == 0u)
                    {
                        pos     = maximum;
                        posPrev = maximum - 1u;
                        posNext = maximum + 1u; 
                    }
                    else
                    {
                        pos     = diplex[maximum];
                        posPrev = diplex[maximum - 1u];
                        posNext = diplex[maximum + 1u];
                        count <<= 1u;
                    }                    
                #elif (CapSense_IS_DIPLEX_SLIDER)
                    /* Calculate next and previous near to maximum */
                    pos     = diplex[maximum];
                    posPrev = diplex[maximum - 1u];
                    posNext = diplex[maximum + 1u];
                    count <<= 1u;                    
                #else                    
                    /* Calculate next and previous near to maximum */
                    if (maximum >= 1u)
                    {
                        posPrev = maximum - 1u;
                    }
                    posNext = maximum + 1u;
                #endif /* (CapSense_IS_DIPLEX_SLIDER && CapSense_IS_NON_DIPLEX_SLIDER) */
                        
                /* Copy Signal to the centroid array */
                #if (CapSense_IS_DIPLEX_SLIDER)
                    CapSense_centroid[CapSense_POS] = startOfSlider[pos];
                #else
                    CapSense_centroid[CapSense_POS] = startOfSlider[maximum];
                #endif /* (CapSense_IS_DIPLEX_SLIDER) */
                    
                /* Check borders for LINEAR Slider */
                if (maximum == 0u)                   /* Start of centroid */
                { 
                    CapSense_centroid[CapSense_POS_PREV] = 0u;
                    CapSense_centroid[CapSense_POS_NEXT] = startOfSlider[posNext];
                }
                else if (maximum == ((count) - 1u)) /* End of centroid */
                {
                    CapSense_centroid[CapSense_POS_PREV] = startOfSlider[posPrev];
                    CapSense_centroid[CapSense_POS_NEXT] = 0u;
                }
                else                                /* Not first Not last */
                {
                    CapSense_centroid[CapSense_POS_PREV] = startOfSlider[posPrev];
                    CapSense_centroid[CapSense_POS_NEXT] = startOfSlider[posNext];
                }
            #endif /* ((CapSense_TOTAL_LINEAR_SLIDERS_COUNT>0u)||(CapSense_TOTAL_TOUCH_PADS_COUNT>0u))*/

        #if (CapSense_ADD_SLIDER_TYPE)
            }
        #endif /* (CapSense_ADD_SLIDER_TYPE) */
    
        /* Subtract noiseThreshold */
        if(CapSense_centroid[CapSense_POS_PREV] > noiseThreshold)
        {
            CapSense_centroid[CapSense_POS_PREV] -= noiseThreshold;
        }
        else
        {
            CapSense_centroid[CapSense_POS_PREV] = 0u;
        }
        
        /* Maximum always greater than fingerThreshold, so greater than noiseThreshold */
        CapSense_centroid[CapSense_POS] -= noiseThreshold;
        
        /* Subtract noiseThreshold */
        if(CapSense_centroid[CapSense_POS_NEXT] > noiseThreshold)
        {
            CapSense_centroid[CapSense_POS_NEXT] -= noiseThreshold;
        }
        else
        {
            CapSense_centroid[CapSense_POS_NEXT] = 0u;
        }
        
        
        /* Si+1 - Si-1 */
        numerator = (uint32) CapSense_centroid[CapSense_POS_NEXT] -
                    (uint32) CapSense_centroid[CapSense_POS_PREV];

        /* Si+1 + Si + Si-1 */
        denominator = (int32) CapSense_centroid[CapSense_POS_PREV] + 
                      (int32) CapSense_centroid[CapSense_POS] + 
                      (int32) CapSense_centroid[CapSense_POS_NEXT];
        
        /* (numerator/denominator) + maximum */
        denominator = (((int32)(uint32)((uint32)numerator << 8u)/denominator) + (int32)(uint32)((uint32) maximum << 8u));

        #if(CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u)
            /* Only required for RADIAL Slider */
            if(denominator < 0)
            {
                denominator += (int32)((uint16)((uint16) count << 8u));
            }
        #endif /* (CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) */
        
        denominator *= (int16)resolution;
        
        /* Round result and put it to uint8 */
        position = ((uint8) HI16((uint32)denominator + CapSense_CENTROID_ROUND_VALUE));

        return (position);
    }    
#endif /* (CapSense_TOTAL_CENTROIDS_COUNT) */


#if((CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u))
    /*******************************************************************************
    * Function Name: CapSense_GetCentroidPos
    ********************************************************************************
    *
    * Summary:
    *  Checks the CapSense_Signal[ ] array for a centroid within
    *  a slider specified range. The centroid position is calculated according to the resolution
    *  specified in the CapSense customizer. The position filters are applied to the
    *  result if enabled.
    *
    * Parameters:
    *  widget:  Widget number.
    *  For every linear slider widget there are defines in this format:
    *  #define CapSense_"widget_name"__LS           5
    * 
    * Return:
    *  Returns a position value of the linear slider.
	*
	* Global Variables:
	*  None
	*
    * Side Effects:
    *  If any sensor within the slider widget is active, the function returns values
    *  from zero to the API resolution value set in the CapSense customizer. If no
    *  sensors are active, the function returns 0xFFFF. If an error occurs during
    *  execution of the centroid/diplexing algorithm, the function returns 0xFFFF.
    *  There are no checks of the widget type argument provided to this function.
    *  The unproper widget type provided will cause unexpected position calculations.
    *
    * Note:
    *  If noise counts on the slider segments are greater than the noise
    *  threshold, this subroutine may generate a false centroid result. The noise
    *  threshold should be set carefully (high enough above the noise level) so
    *  that noise will not generate a false centroid.
    *******************************************************************************/
    uint16 CapSense_GetCentroidPos(uint32 widget)
    {
        #if (CapSense_IS_DIPLEX_SLIDER)
            const uint8 *diplex;
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */
                
        #if (0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)
            uint8 posIndex;
            uint8 firstTimeIndex = CapSense_posFiltersData[widget];
            uint8 posFiltersMask = CapSense_posFiltersMask[widget];  
        #endif /* (0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK) */

        #if ((0u != (CapSense_MEDIAN_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)) || \
             (0u != (CapSense_AVERAGING_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)))
            uint8 tempPos;
        #endif /* ((0u != (CapSense_MEDIAN_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)) || \
               *   (0u != (CapSense_AVERAGING_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)))
               */

        uint8 maximum;
        uint16 position;
        uint8 offset = CapSense_rawDataIndex[widget];
        uint8 count = CapSense_numberOfSensors[widget];
                        
        #if (CapSense_IS_DIPLEX_SLIDER)
            if(widget < CapSense_TOTAL_DIPLEXED_SLIDERS_COUNT)
            {
                maximum = CapSense_diplexTable[widget];
                diplex = &CapSense_diplexTable[maximum];
            }
            else
            {
                diplex = 0u;
            }
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */

		/* Find Maximum within centroid */      
		#if (CapSense_IS_DIPLEX_SLIDER)        
			maximum = CapSense_FindMaximum(offset, count, (uint8)CapSense_fingerThreshold[widget], diplex);
		#else
			maximum = CapSense_FindMaximum(offset, count, (uint8)CapSense_fingerThreshold[widget]);
		#endif /* (CapSense_IS_DIPLEX_SLIDER) */

        if (maximum != 0xFFu)
        {
            /* Calculate centroid */
            position = (uint16) CapSense_CalcCentroid(maximum, 
                         offset, count, CapSense_centroidMult[widget], CapSense_noiseThreshold[widget]);

            #if (0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)
                /* Check if this linear slider has enabled filters */
                if (0u != (posFiltersMask & CapSense_ANY_POS_FILTER))
                {
                    /* Calculate position to store filters data */
                    posIndex  = firstTimeIndex + 1u;
                    
                    if (0u == CapSense_posFiltersData[firstTimeIndex])
                    {
                        /* Init filters */
                        CapSense_posFiltersData[posIndex] = (uint8) position;
                        #if ((0u != (CapSense_MEDIAN_FILTER & \
                                     CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)) || \
                             (0u != (CapSense_AVERAGING_FILTER & \
                                     CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)))

                            if ( (0u != (posFiltersMask & CapSense_MEDIAN_FILTER)) || 
                                 (0u != (posFiltersMask & CapSense_AVERAGING_FILTER)) )
                            {
                                CapSense_posFiltersData[posIndex + 1u] = (uint8) position;
                            }
                        #endif /* ((0u != (CapSense_MEDIAN_FILTER & \
                               *           CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)) || \
                               *   (0u != (CapSense_AVERAGING_FILTER & \
                               *           CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)))
                               */
                        
                        CapSense_posFiltersData[firstTimeIndex] = 1u;
                    }
                    else
                    {
                        /* Do filtering */
                        #if (0u != (CapSense_MEDIAN_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_MEDIAN_FILTER))
                            {
                                tempPos = (uint8) position;
                                position = CapSense_MedianFilter(position,
                                                                (uint16)CapSense_posFiltersData[posIndex],
                                                                (uint16)CapSense_posFiltersData[posIndex + 1u]);
                                CapSense_posFiltersData[posIndex + 1u] =
                                                                             CapSense_posFiltersData[posIndex];
                                CapSense_posFiltersData[posIndex] = tempPos;
                            }
                        #endif /*(0u != (CapSense_MEDIAN_FILTER &
                               *         CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if(0u!=(CapSense_AVERAGING_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_AVERAGING_FILTER)) 
                            {
                                tempPos = (uint8) position;
                                position = CapSense_AveragingFilter(position,
                                                                (uint16)CapSense_posFiltersData[posIndex],
                                                                (uint16)CapSense_posFiltersData[posIndex + 1u]);
                                CapSense_posFiltersData[posIndex+1u]=CapSense_posFiltersData[posIndex];
                                CapSense_posFiltersData[posIndex] = tempPos;
                            }
                        #endif /* (0u != (CapSense_AVERAGING_FILTER & \
                               *           CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_IIR2_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)) 
                            if (0u != (posFiltersMask & CapSense_IIR2_FILTER)) 
                            {
                                position = CapSense_IIR2Filter(position,
                                                                    (uint16)CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }
                        #endif /* (0u != (CapSense_IIR2_FILTER & \
                               *          CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_IIR4_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_IIR4_FILTER))
                            {
                                position = CapSense_IIR4Filter(position,
                                                                    (uint16)CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }                                
                        #endif /* (0u != (CapSense_IIR4_FILTER & \
                               *          CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_JITTER_FILTER & CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_JITTER_FILTER))
                            {
                                position = CapSense_JitterFilter(position,
                                                                    (uint16)CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }
                        #endif /* (0u != (CapSense_JITTER_FILTER & \
                               *           CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK))
                               */
                    }
                }
            #endif /* (0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK) */

        }
        else
        {
            /* Maximum wasn't found */
            position = 0xFFFFu;

            #if(0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK)
                /* Reset filters */
                if(0u != (posFiltersMask & CapSense_ANY_POS_FILTER))
                {
                    CapSense_posFiltersData[firstTimeIndex] = 0u;
                }
            #endif /* (0u != CapSense_LINEAR_SLIDERS_POS_FILTERS_MASK) */
        }

        
        return (position);
    }
#endif /* ((CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u)) */


#if((CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u))
    /*******************************************************************************
    * Function Name: CapSense_GetRadialCentroidPos
    ********************************************************************************
    *
    * Summary:
    *  Checks the CapSense_Signal[ ] array for a centroid within
    *  a slider specified range. The centroid position is calculated according to the resolution
    *  specified in the CapSense customizer. The position filters are applied to the
    *  result if enabled.
    *
    * Parameters:
    *  widget:  Widget number.
    *  For every radial slider widget there are defines in this format:
    *  #define CapSense_"widget_name"__RS           5
    * 
    * Return:
    *  Returns a position value of the radial slider.
    *
	* Global Variables:
	*  None.
	*
    * Side Effects:
    *  If any sensor within the slider widget is active, the function returns values
    *  from zero to the API resolution value set in the CapSense customizer. If no
    *  sensors are active, the function returns 0xFFFF.
    *  There are no checks of the widget type argument provided to this function.
    *  The unproper widget type provided will cause unexpected position calculations.
    *
    * Note:
    *  If noise counts on the slider segments are greater than the noise
    *  threshold, this subroutine may generate a false centroid result. The noise
    *  threshold should be set carefully (high enough above the noise level) so 
    *  that noise will not generate a false centroid.
    *
    *******************************************************************************/
     uint16 CapSense_GetRadialCentroidPos(uint32 widget)
    {
        #if (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)
            uint8 posIndex;
            uint8 firstTimeIndex = CapSense_posFiltersData[widget];
            uint8 posFiltersMask = CapSense_posFiltersMask[widget]; 
        #endif /* (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK) */

        #if ((0u != (CapSense_MEDIAN_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)) || \
             (0u != (CapSense_AVERAGING_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)))
            uint8 tempPos;
        #endif /* ((0u != (CapSense_MEDIAN_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)) || \
               *   (0u != (CapSense_AVERAGING_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)))
               */

        uint8 maximum;
        uint16 position;
        uint8 offset = CapSense_rawDataIndex[widget];
        uint8 count = CapSense_numberOfSensors[widget];

		/* Find Maximum within centroid */      
		#if (CapSense_IS_DIPLEX_SLIDER)        
			maximum = CapSense_FindMaximum(offset, count, (uint8)CapSense_fingerThreshold[widget], 0u);
		#else
			maximum = CapSense_FindMaximum(offset, count, (uint8)CapSense_fingerThreshold[widget]);
		#endif /* (CapSense_IS_DIPLEX_SLIDER) */
        
        if (maximum != CapSense_MAXIMUM_CENTROID)
        {
            /* Calculate centroid */
            position = (uint16) CapSense_CalcCentroid(maximum, 
                         offset, count, CapSense_centroidMult[widget], CapSense_noiseThreshold[widget]);

            #if (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)
                /* Check if this Radial slider has enabled filters */
                if (0u != (posFiltersMask & CapSense_ANY_POS_FILTER))
                {
                    /* Calculate position to store filters data */
                    posIndex  = firstTimeIndex + 1u;
                    
                    if (0u == CapSense_posFiltersData[firstTimeIndex])
                    {
                        /* Init filters */
                        CapSense_posFiltersData[posIndex] = (uint8) position;
                        #if ((0u != (CapSense_MEDIAN_FILTER & \
                                     CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)) || \
                             (0u != (CapSense_AVERAGING_FILTER & \
                                     CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)))

                            if ( (0u != (posFiltersMask & CapSense_MEDIAN_FILTER))  || 
                                 (0u != (posFiltersMask & CapSense_AVERAGING_FILTER)) )
                            {
                                CapSense_posFiltersData[posIndex + 1u] = (uint8) position;
                            }
                        #endif /* ((0u != (CapSense_MEDIAN_FILTER & \
                               *           CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)) || \
                               *   (0u != (CapSense_AVERAGING_FILTER & \
                               *           CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)))
                               */
                        
                        CapSense_posFiltersData[firstTimeIndex] = 1u;
                    }
                    else
                    {
                        /* Do filtering */
                        #if (0u != (CapSense_MEDIAN_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_MEDIAN_FILTER))
                            {
                                tempPos = (uint8) position;
                                position = CapSense_MedianFilter(position,
                                                                        CapSense_posFiltersData[posIndex],
                                                                        CapSense_posFiltersData[posIndex + 1u]);
                                CapSense_posFiltersData[posIndex + 1u] = 
                                                                              CapSense_posFiltersData[posIndex];
                                CapSense_posFiltersData[posIndex] = tempPos;
                            }
                        #endif /* (0u != (CapSense_MEDIAN_FILTER & 
                               *          CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_AVERAGING_FILTER & \
                                    CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_AVERAGING_FILTER))
                            {
                                tempPos = (uint8) position;
                                position = CapSense_AveragingFilter(position, 
                                                                       CapSense_posFiltersData[posIndex],
                                                                       CapSense_posFiltersData[posIndex + 1u]);
                                CapSense_posFiltersData[posIndex+1u]= CapSense_posFiltersData[posIndex];
                                CapSense_posFiltersData[posIndex] = tempPos;
                            }
                        #endif /* (0u != (CapSense_AVERAGING_FILTER & \
                               *          CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_IIR2_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_IIR2_FILTER))
                            {
                                position = CapSense_IIR2Filter(position,
                                                                    (uint16)CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }
                        #endif /* (0u != (CapSense_IIR2_FILTER & 
                               *          CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_IIR4_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_IIR4_FILTER))
                            {
                                position = CapSense_IIR4Filter(position,
                                                                    (uint16)CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }
                        #endif /* (0u != (CapSense_IIR4_FILTER & 
                               *          CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                               */

                        #if (0u != (CapSense_JITTER_FILTER & CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                            if (0u != (posFiltersMask & CapSense_JITTER_FILTER))
                            {
                                position = CapSense_JitterFilter(position, 
                                                                         CapSense_posFiltersData[posIndex]);
                                CapSense_posFiltersData[posIndex] = (uint8) position;
                            }
                        #endif /* (0u != (CapSense_JITTER_FILTER &
                               *           CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK))
                               */
                    }
                }
            #endif /* (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK) */

        }
        else
        {
            /* Maximum wasn't found?? */
            position = 0xFFFFu;

            #if (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK)
                /* Reset filters */
                if((posFiltersMask & CapSense_ANY_POS_FILTER) != 0u)
                {
                    CapSense_posFiltersData[firstTimeIndex] = 0u;
                }
            #endif /* (0u != CapSense_RADIAL_SLIDERS_POS_FILTERS_MASK) */
        }
        
        return (position);
    }
#endif /* ((CapSense_TOTAL_RADIAL_SLIDERS_COUNT > 0u) || (CapSense_TOTAL_LINEAR_SLIDERS_COUNT > 0u)) */


#if(CapSense_TOTAL_TOUCH_PADS_COUNT > 0u)
    /*******************************************************************************
    * Function Name: CapSense_GetTouchCentroidPos
    ********************************************************************************
    *
    * Summary:
    *  If a finger is present on a touchpad, this function calculates the X and Y
    *  position of the finger by calculating the centroids within touchpad specified
    *  range. The X and Y positions are calculated according to the API resolutions set in the
    *  CapSense customizer. Returns 1 if a finger is on the touchpad.
    *  The position filter is applied to the result if enabled.
    *  This function is available only if a touch pad is defined by the CapSense
    *  customizer.
    *
    * Parameters:
    *  widget:  Widget number. 
    *  For every touchpad widget there are defines in this format:
    *  #define CapSense_"widget_name"__TP            5
    *
    *  pos:     Pointer to the array of two uint16 elements, where result
    *  result of calculation of X and Y position are stored.
    *  pos[0u]  - position of X
    *  pos[1u]  - position of Y
    *
    * Return:
    *  Returns a 1 if a finger is on the touch pad, 0 - if not.
	*
	* Global Variables:
	*  None.
	*
    * Side Effects:
    *   There are no checks of the widget type argument provided to this function.
    *   The unproper widget type provided will cause unexpected position
    *   calculations.
    *
    *******************************************************************************/
    uint32 CapSense_GetTouchCentroidPos(uint32 widget, uint16* pos)
    {
        #if (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK)
            uint8 posXIndex;
            uint8 posYIndex;
            uint8 firstTimeIndex = CapSense_posFiltersData[widget];
            uint8 posFiltersMask = CapSense_posFiltersMask[widget];
        #endif /* (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK) */

        #if ((0u != (CapSense_MEDIAN_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK)) || \
             (0u != (CapSense_AVERAGING_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK)))
            uint16 tempPos;
        #endif /* ((0u != (CapSense_MEDIAN_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK)) || \
               *   (0u != (CapSense_AVERAGING_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK)))
               */

        uint8 MaxX;
        uint8 MaxY;
        uint8 posX;
        uint8 posY;
        uint32 touch = 0u;
        uint8 offset = CapSense_rawDataIndex[widget];
        uint8 count = CapSense_numberOfSensors[widget];
        
        /* Find Maximum within X centroid */
        #if (CapSense_IS_DIPLEX_SLIDER)
            MaxX = CapSense_FindMaximum(offset, count, CapSense_fingerThreshold[widget], 0u);
        #else
            MaxX = CapSense_FindMaximum(offset, count, CapSense_fingerThreshold[widget]);
        #endif /* (CapSense_IS_DIPLEX_SLIDER) */

        if (MaxX != CapSense_MAXIMUM_CENTROID)
        {
            offset = CapSense_rawDataIndex[widget + 1u];
            count = CapSense_numberOfSensors[widget + 1u];

            /* Find Maximum within Y centroid */
            #if (CapSense_IS_DIPLEX_SLIDER)
                MaxY = CapSense_FindMaximum(offset, count, CapSense_fingerThreshold[widget + 1u], 0u);
            #else
                MaxY = CapSense_FindMaximum(offset, count, CapSense_fingerThreshold[widget + 1u]);
            #endif /* (CapSense_IS_DIPLEX_SLIDER) */

            if (MaxY != CapSense_MAXIMUM_CENTROID)
            {
                /* X and Y maximums are found = true touch */
                touch = 1u;
                
                /* Calculate Y centroid */
                posY = CapSense_CalcCentroid(MaxY, offset, count, 
                            CapSense_centroidMult[widget + 1u], CapSense_noiseThreshold[widget + 1u]);
                
                /* Calculate X centroid */
                offset = CapSense_rawDataIndex[widget];
                count = CapSense_numberOfSensors[widget];
                
                posX = CapSense_CalcCentroid(MaxX, offset, count, 
                            CapSense_centroidMult[widget],CapSense_noiseThreshold[widget]);
    
                #if (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK)
                    /* Check if this TP has enabled filters */
                    if (0u != (posFiltersMask & CapSense_ANY_POS_FILTER))
                    {
                        /* Calculate position to store filters data */
                        posXIndex  = firstTimeIndex + 1u;
                        posYIndex  = CapSense_posFiltersData[widget + 1u];
                        
                        if (0u == CapSense_posFiltersData[firstTimeIndex])
                        {
                            /* Init filters */
                            CapSense_posFiltersData[posXIndex] = posX;
                            CapSense_posFiltersData[posYIndex] = posY;

                            #if((0u != (CapSense_MEDIAN_FILTER & \
                                        CapSense_TOUCH_PADS_POS_FILTERS_MASK))|| \
                                (0u != (CapSense_AVERAGING_FILTER & \
                                        CapSense_TOUCH_PADS_POS_FILTERS_MASK)))

                                if ( (0u != (posFiltersMask & CapSense_MEDIAN_FILTER)) || 
                                     (0u != (posFiltersMask & CapSense_AVERAGING_FILTER)) )
                                {
                                    CapSense_posFiltersData[posXIndex + 1u] = posX;
                                    CapSense_posFiltersData[posYIndex + 1u] = posY;
                                }
                            #endif /* ((0u != (CapSense_MEDIAN_FILTER & \
                                   *           CapSense_TOUCH_PADS_POS_FILTERS_MASK)) || \
                                   *    (0u != (CapSense_AVERAGING_FILTER & \
                                   *            CapSense_TOUCH_PADS_POS_FILTERS_MASK)))
                                   */
                            
                            CapSense_posFiltersData[firstTimeIndex] = 1u;
                        }
                        else
                        {
                            /* Do filtering */
                            #if (0u != (CapSense_MEDIAN_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                if (0u != (posFiltersMask & CapSense_MEDIAN_FILTER))
                                {
                                    tempPos = posX;
                                    posX = (uint8) CapSense_MedianFilter(posX,
                                                                      CapSense_posFiltersData[posXIndex],
                                                                      CapSense_posFiltersData[posXIndex + 1u]);
                                    CapSense_posFiltersData[posXIndex + 1u] = 
                                                                             CapSense_posFiltersData[posXIndex];
                                    CapSense_posFiltersData[posXIndex] = tempPos;
                                    
                                    tempPos = posY;
                                    posY = (uint8) CapSense_MedianFilter(posY,
                                                                       CapSense_posFiltersData[posYIndex], 
                                                                       CapSense_posFiltersData[posYIndex + 1u]);
                                    CapSense_posFiltersData[posYIndex + 1u] = 
                                                                             CapSense_posFiltersData[posYIndex];
                                    CapSense_posFiltersData[posYIndex] = tempPos;
                                }
                                
                            #endif /* (0u != (CapSense_MEDIAN_FILTER & \
                                   *          CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                   */

                            #if(0u !=(CapSense_AVERAGING_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                if (0u != (posFiltersMask & CapSense_AVERAGING_FILTER))
                                {
                                    tempPos = posX;
                                    posX = (uint8) CapSense_AveragingFilter(posX,
                                                                       CapSense_posFiltersData[posXIndex], 
                                                                       CapSense_posFiltersData[posXIndex + 1u]);
                                    CapSense_posFiltersData[posXIndex + 1u] = 
                                                                             CapSense_posFiltersData[posXIndex];
                                    CapSense_posFiltersData[posXIndex] = tempPos;
                                    
                                    tempPos = posY;
                                    posY = (uint8) CapSense_AveragingFilter(posY, 
                                                                      CapSense_posFiltersData[posYIndex], 
                                                                      CapSense_posFiltersData[posYIndex + 1u]);
                                    CapSense_posFiltersData[posYIndex + 1u] = 
                                                                            CapSense_posFiltersData[posYIndex];
                                    CapSense_posFiltersData[posYIndex] = tempPos;
                                }

                            #endif /* (0u != (CapSense_AVERAGING_FILTER & \
                                   *           CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                   */

                            #if (0u != (CapSense_IIR2_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                if (0u != (posFiltersMask & CapSense_IIR2_FILTER))
                                {
                                    posX = (uint8) CapSense_IIR2Filter(posX, 
                                                                           CapSense_posFiltersData[posXIndex]);
                                    CapSense_posFiltersData[posXIndex] = posX;
                                    
                                    posY = (uint8) CapSense_IIR2Filter(posY, 
                                                                            CapSense_posFiltersData[posYIndex]);
                                    CapSense_posFiltersData[posYIndex] = posY;
                                }
                                
                            #endif /* (0u != (CapSense_IIR2_FILTER & \
                                   *          CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                   */

                            #if (0u != (CapSense_IIR4_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                if (0u != (posFiltersMask & CapSense_IIR4_FILTER))
                                {
                                    posX = (uint8) CapSense_IIR4Filter((uint16)posX,
                                                                    (uint16)CapSense_posFiltersData[posXIndex]);
                                    CapSense_posFiltersData[posXIndex] = posX;

                                    posY = (uint8) CapSense_IIR4Filter((uint16)posY,
                                                                    (uint16)CapSense_posFiltersData[posYIndex]);
                                    CapSense_posFiltersData[posYIndex] = posY;
                                }
                                
                            #endif /* (0u != (CapSense_IIR4_FILTER & \
                                   *           CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                   */

                            #if (0u != (CapSense_JITTER_FILTER & CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                if (0u != (posFiltersMask & CapSense_JITTER_FILTER))
                                    {
                                        posX = (uint8) CapSense_JitterFilter(posX, 
                                                                            CapSense_posFiltersData[posXIndex]);
                                        CapSense_posFiltersData[posXIndex] = posX;
                                        
                                        posY = (uint8) CapSense_JitterFilter(posY, 
                                                                            CapSense_posFiltersData[posYIndex]);
                                        CapSense_posFiltersData[posYIndex] = posY;
                                    }
                            #endif /* (0u != (CapSense_JITTER_FILTER & \
                                   *           CapSense_TOUCH_PADS_POS_FILTERS_MASK))
                                   */
                        }
                    }
                #endif /* (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK) */

                /* Save positions */
                pos[0u] = posX;
                pos[1u] = posY;
            }
        }

        #if (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK)
            if(touch == 0u)
            {
                /* Reset filters */
                if ((posFiltersMask & CapSense_ANY_POS_FILTER) != 0u)
                {
                    CapSense_posFiltersData[firstTimeIndex] = 0u;
                }
            }
        #endif /* (0u != CapSense_TOUCH_PADS_POS_FILTERS_MASK) */
        
        return (touch);
    }
#endif /* (CapSense_TOTAL_TOUCH_PADS_COUNT > 0u) */


#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_MEDIAN_FILTER)) || \
      ((CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)) )
    /*******************************************************************************
    * Function Name: CapSense_MedianFilter
    ********************************************************************************
    *
    * Summary:
    *  This is the Median filter function. 
    *  The median filter looks at the three most recent samples and reports the 
    *  median value.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *  x3:  Before previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_MedianFilter(uint16 x1, uint16 x2, uint16 x3)
    {
        uint16 tmp;
        
        if (x1 > x2)
        {
            tmp = x2;
            x2 = x1;
            x1 = tmp;
        }
        
        if (x2 > x3)
        {
            x2 = x3;
        }
        
        return ((x1 > x2) ? x1 : x2);
    }
#endif /* ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_MEDIAN_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_MEDIAN_FILTER)) || \
      ((CapSense_TUNING_METHOD == CapSense__TUNING_AUTO)) ) */


#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_AVERAGING_FILTER)) )
    /*******************************************************************************
    * Function Name: CapSense_AveragingFilter
    ********************************************************************************
    *
    * Summary:
    *  This is the Averaging filter function.
    *  The averaging filter looks at the three most recent samples of a position and
    *  reports the averaging value.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *  x3:  Before previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_AveragingFilter(uint16 x1, uint16 x2, uint16 x3)
    {
        uint32 tmp = ((uint32)x1 + (uint32)x2 + (uint32)x3) / 3u;
        
        return ((uint16) tmp);
    }
#endif /* ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_AVERAGING_FILTER) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_AVERAGING_FILTER) ) */


#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR2_FILTER)) )
    /*******************************************************************************
    * Function Name: CapSense_IIR2Filter
    ********************************************************************************
    *
    * Summary:
    *  This is the IIR1/2 filter function. IIR1/2 = 1/2current + 1/2previous.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_IIR2Filter(uint16 x1, uint16 x2)
    {
        uint32 tmp;
        
        /* IIR = 1/2 Current Value+ 1/2 Previous Value */
        tmp = (uint32)x1 + (uint32)x2;
        tmp >>= 1u;
    
        return ((uint16) tmp);
    }
#endif /* ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR2_FILTER)) || \
       *    (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR2_FILTER)) )
       */


#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR4_FILTER)) )
    /*******************************************************************************
    * Function Name: CapSense_IIR4Filter
    ********************************************************************************
    *
    * Summary:
    *  This is the IIR1/4 filter function. IIR1/4 = 1/4current + 3/4previous.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *
    * Return:
    *  Returns a filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_IIR4Filter(uint16 x1, uint16 x2)
    {
        uint32 tmp;
        
        /* IIR = 1/4 Current Value + 3/4 Previous Value */
        tmp = (uint32)x1 + (uint32)x2;
        tmp += ((uint32)x2 << 1u);
        tmp >>= 2u;
        
        return ((uint16) tmp);
    }
#endif /* ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR4_FILTER)) || \
       *    (0u != (CapSense_POS_FILTERS_MASK & CapSense_IIR4_FILTER)) )
       */


#if ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER)) || \
      (0u != (CapSense_POS_FILTERS_MASK & CapSense_JITTER_FILTER)) )
    /*******************************************************************************
    * Function Name: uint16 CapSense_JitterFilter
    ********************************************************************************
    *
    * Summary:
    *  This is the Jitter filter function.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_JitterFilter(uint16 x1, uint16 x2)
    {
        if (x1 > x2)
        {
            x1--;
        }
        else
        {
            if (x1 < x2)
            {
                x1++;
            }
        }
    
        return x1;
    }
#endif /* ( (0u != (CapSense_RAW_FILTER_MASK & CapSense_JITTER_FILTER)) || \
       *    (0u != (CapSense_POS_FILTERS_MASK & CapSense_JITTER_FILTER)) )
       */


#if (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER))
    /*******************************************************************************
    * Function Name: CapSense_IIR8Filter
    ********************************************************************************
    *
    * Summary:
    *  This is the IIR1/8 filter function. IIR1/8 = 1/8current + 7/8previous.
    *  Only applies for raw data.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_IIR8Filter(uint16 x1, uint16 x2)
    {
        uint32 tmp;
        
        /* IIR = 1/8 Current Value + 7/8 Previous Value */
        tmp = (uint32)x1;
        tmp += (((uint32)x2 << 3u) - ((uint32)x2));
        tmp >>= 3u;
    
        return ((uint16) tmp);
    }
#endif /* (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR8_FILTER)) */


#if (0u != (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER))
    /*******************************************************************************
    * Function Name: CapSense_IIR16Filter
    ********************************************************************************
    *
    * Summary:
    *  This is the IIR1/16 filter function. IIR1/16 = 1/16current + 15/16previous.
    *  Only applies for raw data.
    *
    * Parameters:
    *  x1:  Current value.
    *  x2:  Previous value.
    *
    * Return:
    *  Returns filtered value.
	*
	* Global Variables:
	*  None.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint16 CapSense_IIR16Filter(uint16 x1, uint16 x2)
    {
        uint32 tmp;
        
        /* IIR = 1/16 Current Value + 15/16 Previous Value */
        tmp = (uint32)x1;
        tmp += (((uint32)x2 << 4u) - ((uint32)x2));
        tmp >>= 4u;
        
        return ((uint16) tmp);
    }
#endif /* (CapSense_RAW_FILTER_MASK & CapSense_IIR16_FILTER) */


#if (0u != (CapSense_TOTAL_MATRIX_BUTTONS_COUNT))

    /*******************************************************************************
    * Function Name: CapSense_GetMatrixButtonPos
    ********************************************************************************
    *
    * Summary:
    *  Function calculates and returns a touch position (column and row) for the matrix
    *  button widget.
    *
    * Parameters:
    *  widget:  widget number;
    *  pos:     pointer to an array of two uint8, where touch position will be 
    *           stored:
    *           pos[0] - column position;
    *           pos[1] - raw position.
    *
    * Return:
    *  Returns 1 if row and column sensors of matrix button are active, 0 - in other
    *  cases.
	*
	* Global Variables:
	*  CapSense_fingerThreshold[ ] � used to store the finger threshold for all widgets.
	*  CapSense_sensorSignal[ ] � used to store a difference between the current value of 
	*  raw data and a previous value of the baseline.
	*
	* Side Effects:
	*  None
	* 
    *******************************************************************************/
    uint32 CapSense_GetMatrixButtonPos(uint32 widget, uint8* pos)
    {
        uint8 i;
        uint32 retVal = 0u;
        uint16 row_sig_max = 0u;
        uint16 col_sig_max = 0u;
        uint8 row_ind = 0u;
        uint8 col_ind = 0u;

        if (CapSense_CheckIsWidgetActive(widget) == 1u)
        {
            /* Find row number with maximal signal value */
            for(i = CapSense_rawDataIndex[widget]; i < (CapSense_rawDataIndex[widget] + \
                 CapSense_numberOfSensors[widget]); i++)
            {
                if (CapSense_sensorSignal[i] > col_sig_max)
                {
                    col_ind = i;
                    col_sig_max = CapSense_sensorSignal[i];
                }
            }

            /* Find row number with maximal signal value */
            for(i = CapSense_rawDataIndex[widget+1u]; i < (CapSense_rawDataIndex[widget+1u] + \
                 CapSense_numberOfSensors[widget+1u]); i++)
            {
                if (CapSense_sensorSignal[i] > row_sig_max)
                {
                    row_ind = i;
                    row_sig_max = CapSense_sensorSignal[i];
                }
            }

            if((col_sig_max >= CapSense_fingerThreshold[widget]) && \
               (row_sig_max >= CapSense_fingerThreshold[widget+1u]))
            {
                pos[0u] = col_ind - CapSense_rawDataIndex[widget];
                pos[1u] = row_ind - CapSense_rawDataIndex[widget+1u];
                retVal = 1u;
            }
        }
        return (retVal);
    }

#endif /* (0u != (CapSense_TOTAL_MATRIX_BUTTONS_COUNT)) */

/*******************************************************************************
* Function Name: CapSense_GetWidgetNumber
********************************************************************************
*
* Summary:
*  This API returns the widget number for the sensor.
*
* Parameters:
*  sensor: sensor index. The value of index can be 
*  from 0 to (CapSense_TOTAL_SENSOR_COUNT-1).
*
* Return:
*  This API returns the widget number for the sensor. 
*
* Global Variables:
*  CapSense_widgetNumber[]  - stores widget numbers.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint32 CapSense_GetWidgetNumber(uint32 sensor)
{
	return((uint32)CapSense_widgetNumber[sensor]);
}

/*******************************************************************************
* Function Name: CapSense_GetLowBaselineReset
********************************************************************************
*
* Summary:
*  This API returns the low baseline reset threshold value for the  sensor.
*
* Parameters:
*  sensor: sensor index. The value of index can be 
*  from 0 to (CapSense_TOTAL_SENSOR_COUNT-1).
*
* Return:
*  low baseline reset threshold value a sensor.
*
* Global Variables:
*  CapSense_lowBaselineReset[]  - stores low baseline reset values.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetLowBaselineReset(uint32 sensor)
{
	return(CapSense_lowBaselineReset[sensor]);
}

/*******************************************************************************
* Function Name: CapSense_GetDebounce
********************************************************************************
*
* Summary:
*  This API returns a debounce value.
*
* Parameters:
*  sensor: sensor index. The value of index can be 
*  from 0 to (CapSense_TOTAL_SENSOR_COUNT-1).
*
* Return:
*  Debounce value 
*
* Global Variables:
*  CapSense_debounce[]  - stores the debounce value.
* 
* Side Effects:
*  None
*
*******************************************************************************/
uint8 CapSense_GetDebounce(uint32 widget)
{
	return(CapSense_debounce[widget]);
}

/* [] END OF FILE */
