/**
 * @defgroup Time Time functions
 * @brief Functions for fetching time over NTP
 *
 */
/*@{*/
/**
 * @file time.h
 * @author Otso Jousimaa <otso@ojousima.net>
 * @date 2019-03-14
 * @copyright Ruuvi Innovations Ltd, license BSD-3-Clause.
 *
 */

/**
 * @brief get time from a pool of NTP servers
 * 
 * Time is refreshed once per 24 hours or when @ref time_sync is called.
 *
 * @param[in] param unused
 */
void time_task(void *param);

/**
 * @brief trigger synchronizing time.
 */
void time_sync(void);


/*@}*/