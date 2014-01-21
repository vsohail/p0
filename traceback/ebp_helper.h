/** @file ebp_helper.h
 *
 *  This file conatins the assemply routines required by traceback
 *  to get the required information for stack crawling
 *
 *  @author Sohil Habib (snhabib)
 *  @bug No known bugs.
 */

/** @brief ebp_addr gets the value of ebp, or address of the old ebp
 *
 *  The assembly instruction for this function just copies the value of ebp into
 *  the %eax and returns
 *
 *  @param Void.
 *  @return void* the address of old ebp on stack
 */
void* ebp_addr();
