#include "max17048.h"

#include "i2c.h"

#include <stdint.h>

// MAX17048 i2c address
#define MAX_ADDR 0x36U

// MAX17048 registers
#define VCELL 0x02U     // R/-: ADC measurement of VCELL, 78.125uV/cell
#define SOC 0x04U       // R/-: Battery state of charge, 1%/256
#define MODE 0x06U      // -/W: Current operating mode, enable sleep
#define VERSION 0x08U   // R/-: Production version
#define HIBRT 0x0AU     // R/W: Hibernation thresholds
#define CONFIG 0x0CU    // R/W: Compensation, toggle sleep, alert masks, config
#define VALRT 0x14U     // R/W: Voltage level to generate alert
#define CRATE 0x16U     // R/-: Charge/discharge rate, 0.208%/hr
#define VRESET_ID 0x18U // R/W: VCELL for chip reset
#define STATUS 0x1AU    // R/W: Over/undervoltage, SOC change/low, reset alerts
#define TABLE 0x40U     // -/W: Configures battery parameters
#define CMD 0xFEU       // R/W: POR command

// MAX17048 masks/constants
#define VERSION_MSK 0xFFF0U
#define PART_NUMBER 0x0010U

#define BAT_LOW_POS 0U
#define BAT_LOW_MSK (0x001FU << BAT_LOW_POS)
#define BAT_LOW_MIN 1U
#define BAT_LOW_MAX 32U

#define ALRT_BIT_POS 5U
#define ALRT_BIT_MSK (0x0001U << ALRT_BIT_POS)

#define ALSC_BIT_POS 6U
#define ALSC_BIT_MSK (0x0001U << ALSC_BIT_POS)

#define VALRT_MAX_POS 0U
#define VALRT_MAX_MSK (0x00FFU << VALRT_MAX_POS)
#define VALRT_MIN_POS 8U
#define VALRT_MIN_MSK (0x00FFU << VALRT_MIN_POS)
#define VALRT_RESOLUTION 20U

#define VRESET_POS 9U
#define VRESET_MSK (0x007FU << VRESET_POS)
#define VRESET_RESOLUTION 40U

#define ENVR_BIT_POS 14U
#define ENVR_BIT_MSK (0x0001U << ENVR_BIT_POS)

#define ALRT_STATUS_POS 8U
#define ALRT_STATUS_MSK (0x003FU << ALRT_STATUS_POS)

#define VCELL_TO_MV(vcell) ((vcell * 5U) >> 6U)

// MAX17048 uses big endian register layout
#define SWAP16(x) ((uint16_t)(((x) << 8U) | ((x) >> 8U)))

static inline bool read_reg(uint8_t reg, uint16_t* out) {
    if (!i2c_master_read_u16(MAX_ADDR, reg, out)) {
        return false;
    }

    *out = SWAP16(*out);
    return true;
}

static inline bool write_reg(uint8_t reg, uint16_t data) {
    return i2c_master_write_u16(MAX_ADDR, reg, SWAP16(data));
}

static inline bool modify_reg(uint8_t reg, uint16_t data, uint16_t mask) {
    uint16_t buf;
    if (!read_reg(reg, &buf)) {
        return false;
    }

    buf = (buf & ~mask) | (data & mask);
    return write_reg(reg, buf);
}

bool max17048_is_present(void) {
    uint16_t data;
    if (!read_reg(VERSION, &data)) {
        return false;
    }

    return ((data & VERSION_MSK) == PART_NUMBER);
}

bool max17048_get_vcell(max17048_voltage_t* mv) {
    if (!mv) return false;

    uint16_t data;
    if (!read_reg(VCELL, &data)) {
        return false;
    }

    *mv = (max17048_voltage_t)(VCELL_TO_MV(data));
    return true;
}

bool max17048_get_soc(max17048_soc_t* percent) {
    if (!percent) return false;

    uint16_t data;
    if (!read_reg(SOC, &data)) {
        return false;
    }

    *percent = (max17048_soc_t)(data >> 8);
    return true;
}

bool max17048_set_bat_low_soc(max17048_soc_t percent) {
    if (percent < BAT_LOW_MIN || percent > BAT_LOW_MAX) {
        return false;
    }
    uint16_t data = (uint16_t)((BAT_LOW_MAX - (percent % BAT_LOW_MAX)) & BAT_LOW_MSK);

    return modify_reg(CONFIG, data, BAT_LOW_MSK);
}

bool max17048_set_undervolted_voltage(max17048_voltage_t mv) {
    uint16_t data = (uint16_t)(((mv / VALRT_RESOLUTION) << VALRT_MIN_POS) & VALRT_MIN_MSK);

    return modify_reg(VALRT, data, VALRT_MIN_MSK);
}

bool max17048_set_overvolted_voltage(max17048_voltage_t mv) {
    uint16_t data = (uint16_t)(((mv / VALRT_RESOLUTION) << VALRT_MAX_POS) & VALRT_MAX_MSK);

    return modify_reg(VALRT, data, VALRT_MAX_MSK);
}

bool max17048_set_reset_voltage(max17048_voltage_t mv) {
    uint16_t data = (uint16_t)(((mv / VRESET_RESOLUTION) << VRESET_POS) & VRESET_MSK);

    return modify_reg(VRESET_ID, data, VRESET_MSK);
}

bool max17048_set_soc_change_alert(bool enable) {
    uint16_t data = (uint16_t)((enable << ALSC_BIT_POS) & ALSC_BIT_MSK);

    return modify_reg(CONFIG, data, ALSC_BIT_MSK);
}

bool max17048_set_voltage_reset_alert(bool enable) {
    uint16_t data = (uint16_t)((enable << ENVR_BIT_POS) & ENVR_BIT_MSK);

    return modify_reg(STATUS, data, ENVR_BIT_MSK);
}

bool max17048_clear_alerts(void) {
    bool ok = true;

    if (ok) ok = modify_reg(STATUS, 0, ALRT_STATUS_MSK);
    if (ok) ok = modify_reg(CONFIG, 0, ALRT_BIT_MSK);

    return ok;
}

bool max17048_get_alerts(max17048_alert_t* alerts) {
    if (!alerts) return false;

    bool ok = true;
    uint16_t data;

    if (ok) ok = read_reg(STATUS, &data);
    if (ok) ok = max17048_clear_alerts();

    *alerts = (max17048_alert_t)((data & ALRT_STATUS_MSK) >> ALRT_STATUS_POS);
    return true;
}
