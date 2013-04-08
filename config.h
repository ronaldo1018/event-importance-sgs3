#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_TURN_ON_ALLOCATION 1
#define CONFIG_TURN_ON_DPM 1
#define CONFIG_TURN_ON_MIGRATION 1
#define CONFIG_TURN_ON_PRIORITIZE 1
#define CONFIG_TURN_ON_DVFS 1
#define CONFIG_TURN_ON_START_AT_MID 0
#define CONFIG_TURN_ON_AGING 0

// timer
#define CONFIG_HZ 200
#define JIFFIES_TIME(x)             ((int)(CONFIG_HZ * x))
#define UTILIZATION_SAMPLING_TIME   JIFFIES_TIME(0.1)
#define TMP_MID_TIME                JIFFIES_TIME(0.15)
#define TMP_HIGH_TIME               JIFFIES_TIME(0.5)
#define INITIAL_AGING_TIME          JIFFIES_TIME(1) // only used when initialize, aging time will change based on number of low importance threads
#define CHECK_ACTIVITY_TIME			5
// DVFS
#define UTILIZATION_PROMOTION_RATIO 1
#define CONFIG_IS_SAMSUNG_FREQUENCY_TABLE 1
#define CONFIG_SCALE_UP_RATIO 0.5
// DPM
#define THRESHOLD2 500000 /* switch threshold of 1 or 2 core*/
#define THRESHOLD3 700000 /* switch threshold of 2 or 3 core*/
#define THRESHOLD4 900000 /* switch threshold of 3 or 4 core*/
#define NUM_OF_CORE 4
// touch
#define CONFIG_POLLING_TOUCH_STATUS 0
#define TOUCH_INTERRUPT_DRIVER "melfas-ts"
#endif // __CONFIG_H__
