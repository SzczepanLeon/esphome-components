
/*
 * Apator INT40 / E.ITN 40
 * Custom ESPHome wM-Bus driver
 *
 * Manufacturer: APA
 * Version: 0D
 * Device type: 08
 * CI: A0
 */

#include "meters_common_implementation.h"

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);

        void processContent(Telegram *t);

        uint16_t read16(
            const std::vector<uchar> &data,
            size_t pos
        );

        std::string dateToString(uint16_t raw);
    };

    static bool ok = registerDriver([](DriverInfo &di)
    {
        di.setName("apatoreitn40");

        di.setDefaultFields(
            "name,id,current_date,season_start_date,"
            "current_hca,previous_hca,"
            "current_month_hca,"
            "previous_month_hca,"
            "previous_month_02_hca,"
            "previous_month_03_hca,"
            "previous_month_04_hca,"
            "previous_month_05_hca,"
            "previous_month_06_hca,"
            "previous_month_07_hca,"
            "previous_month_08_hca,"
            "previous_month_09_hca,"
            "previous_month_10_hca,"
            "previous_month_11_hca,"
            "previous_month_12_hca,"
            "previous_month_13_hca,"
            "previous_month_14_hca,"
            "previous_month_15_hca,"
            "previous_2y_hca,previous_3y_hca,"
            "temp_radiator_c,temp_room_c,status,timestamp"
        );

        di.setMeterType(MeterType::HeatCostAllocationMeter);

        // Manufacturer APA, version 0D, device type 08.
        di.addDetection(MANUFACTURER_APA, 0x0D, 0x08);

        di.usesProcessContent();

        di.setConstructor([](MeterInfo &mi, DriverInfo &di)
        {
            return std::shared_ptr<Meter>(
                new Driver(mi, di)
            );
        });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di)
        : MeterCommonImplementation(mi, di)
    {
        addStringField(
            "current_date",
            "Current date reported by meter.",
            DEFAULT_PRINT_PROPERTIES
        );

        addStringField(
            "season_start_date",
            "Annual billing season start date.",
            DEFAULT_PRINT_PROPERTIES
        );

        addNumericField(
            "current_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Current billing period consumption."
        );

        addNumericField(
            "previous_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Previous billing period consumption."
        );

        addNumericField(
            "current_month_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Current month consumption."
        );

        addNumericField(
            "previous_month_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Previous closed month consumption."
        );

        for (int i = 2; i <= 15; i++)
        {
            char field[32];

            snprintf(
                field,
                sizeof(field),
                "previous_month_%02d_hca",
                i
            );

            addNumericField(
                field,
                Quantity::HCA,
                DEFAULT_PRINT_PROPERTIES,
                "Historical monthly consumption."
            );
        }

        addNumericField(
            "previous_2y_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Consumption two billing periods ago."
        );

        addNumericField(
            "previous_3y_hca",
            Quantity::HCA,
            DEFAULT_PRINT_PROPERTIES,
            "Consumption three billing periods ago."
        );

        addNumericField(
            "temp_radiator_c",
            Quantity::Temperature,
            DEFAULT_PRINT_PROPERTIES,
            "Radiator surface temperature."
        );

        addNumericField(
            "temp_room_c",
            Quantity::Temperature,
            DEFAULT_PRINT_PROPERTIES,
            "Ambient room temperature."
        );

        addStringField(
            "status",
            "Seal and sensor status.",
            DEFAULT_PRINT_PROPERTIES
        );
    }

    uint16_t Driver::read16(
        const std::vector<uchar> &data,
        size_t pos
    )
    {
        if (pos + 1 >= data.size())
            return 0;

        return static_cast<uint16_t>(data[pos])
             | (static_cast<uint16_t>(data[pos + 1]) << 8);
    }

    std::string Driver::dateToString(uint16_t raw)
    {
        if (raw == 0)
            return "";

        uint day   = raw & 0x1F;
        uint month = (raw >> 5) & 0x0F;
        uint year  = ((raw >> 9) & 0x1F) + 2000;

        if (day < 1 || day > 31 || month < 1 || month > 12)
            return "";

        char buf[32];

        snprintf(
            buf,
            sizeof(buf),
            "%04u-%02u-%02uT02:00:00Z",
            year,
            month,
            day
        );

        return buf;
    }

    void Driver::processContent(Telegram *t)
    {
        std::vector<uchar> content;
        t->extractPayload(&content);

        ESP_LOGD(
            "apatoreitn40",
            "CI=0x%02X, payload length=%u",
            t->tpl_ci,
            static_cast<unsigned>(content.size())
        );

        debugPayload(
            "(apatoreitn40) raw payload",
            content
        );

        // Handle CI B6 header, if present.
        if (t->tpl_ci == 0xB6)
        {
            if (content.empty())
                return;

            uint header_len = content[0] + 1;

            if (header_len > content.size())
            {
                debugPayload(
                    "(apatoreitn40) invalid header",
                    content
                );
                return;
            }

            content.erase(
                content.begin(),
                content.begin() + header_len
            );
        }

        // For CI A0, add CI byte to the beginning.
        if (t->tpl_ci == 0xA0)
            content.insert(content.begin(), t->tpl_ci);

        ESP_LOGD(
            "apatoreitn40",
            "payload after CI handling: %u bytes",
            static_cast<unsigned>(content.size())
        );

        // CI (1 byte) + 27 words (54 bytes).
        if (content.size() != 55 || content[0] != 0xA0)
        {
            debugPayload(
                "(apatoreitn40) unexpected payload",
                content
            );
            return;
        }

        // First word begins at byte 3.
        const size_t first_word = 3;

        auto word = [&](int word_index) -> uint16_t
        {
            return read16(
                content,
                first_word + 2 * (word_index - 1)
            );
        };

        // Diagnostic: display all 27 words.
        for (int i = 1; i <= 27; i++)
        {
            uint16_t value = word(i);

            ESP_LOGD(
                "apatoreitn40",
                "word[%02d] = %u (0x%04X)",
                i,
                value,
                value
            );
        }

        // Word 1: annual season start date.
        std::string season_date = dateToString(word(1));

        setStringValue(
            "season_start_date",
            season_date,
            NULL
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET season_start_date = %s",
            season_date.c_str()
        );

        // Words 2 and 3: temperature values.
        double radiator_temp = word(2) / 16.0;
        double room_temp = word(3) / 16.0;

        setNumericValue(
            "temp_radiator_c",
            Unit::C,
            radiator_temp
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET temp_radiator_c = %.2f",
            radiator_temp
        );

        setNumericValue(
            "temp_room_c",
            Unit::C,
            room_temp
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET temp_room_c = %.2f",
            room_temp
        );

        // Word 4: current month.
        uint16_t current_month = word(4);

        setNumericValue(
            "current_month_hca",
            Unit::HCA,
            current_month
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET current_month_hca = %u",
            current_month
        );

        // Word 5: previous month.
        uint16_t previous_month = word(5);

        setNumericValue(
            "previous_month_hca",
            Unit::HCA,
            previous_month
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET previous_month_hca = %u",
            previous_month
        );

        // Words 6-19: previous months 2-15.
        for (int i = 2; i <= 15; i++)
        {
            char field[32];

            snprintf(
                field,
                sizeof(field),
                "previous_month_%02d_hca",
                i
            );

            uint16_t value = word(4 + i);

            setNumericValue(
                field,
                Unit::HCA,
                value
            );

            ESP_LOGD(
                "apatoreitn40",
                "SET %s = %u",
                field,
                value
            );
        }

        // Words 20-23: current and historical billing periods.
        uint16_t current_hca = word(20);
        uint16_t previous_hca = word(21);
        uint16_t previous_2y = word(22);
        uint16_t previous_3y = word(23);

        setNumericValue(
            "current_hca",
            Unit::HCA,
            current_hca
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET current_hca = %u",
            current_hca
        );

        setNumericValue(
            "previous_hca",
            Unit::HCA,
            previous_hca
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET previous_hca = %u",
            previous_hca
        );

        setNumericValue(
            "previous_2y_hca",
            Unit::HCA,
            previous_2y
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET previous_2y_hca = %u",
            previous_2y
        );

        setNumericValue(
            "previous_3y_hca",
            Unit::HCA,
            previous_3y
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET previous_3y_hca = %u",
            previous_3y
        );

        // Word 24: current date.
        std::string current_date = dateToString(word(24));

        setStringValue(
            "current_date",
            current_date,
            NULL
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET current_date = %s",
            current_date.c_str()
        );

        // Word 26: seal and sensor status.
        uint16_t status = word(26);

        const char *status_text =
            status == 0x0000 ? "OK" :
            status == 0xFFFF ? "SEAL_BROKEN" :
                               "FAULT";

        setStringValue(
            "status",
            status_text,
            NULL
        );

        ESP_LOGD(
            "apatoreitn40",
            "SET status = %s (0x%04X)",
            status_text,
            status
        );
    }
}

KEEP_DRIVER(apatoreitn40);
