/**
 * @file rf.h
 * @author Robin Emley (www.Mk2PVrouter.co.uk)
 * @author Frederic Metrich (frederic.metrich@live.fr)
 * @author Emmanuel Havet (@sosandroid)
 * @brief rfm12 and rfm69 related confs
 * @date 2022-12-29
 */
 
// (for the RFM12B module)
//  frequency options are RF12_433MHZ, RF12_868MHZ or RF12_915MHZ

#define RF69_COMPAT 0 // for the RFM12B
// #define RF69_COMPAT 1 // for the RF69
#include <JeeLib.h>
#define FREQ RF12_868MHZ

constexpr int nodeID{10};        /**<  RFM12B node ID */
constexpr int networkGroup{210}; /**< wireless network group - needs to be same for all nodes */
constexpr int UNO{1};            /**< for when the processor contains the UNO bootloader. */