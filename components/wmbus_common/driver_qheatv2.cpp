/*
 Copyright (C) 2026 Fredrik Öhrström (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"meters_common_implementation.h"
#include"manufacturer_specificities.h"
#include<algorithm>
#include<cctype>

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);

    protected:
        void processContent(Telegram *t) override;
    };

    static bool ok = registerDriver([](DriverInfo&di)
    {
        di.setName("qheatv2");
        di.setDefaultFields("name,id,status,total_kwh,timestamp");
        di.setMeterType(MeterType::HeatMeter);
        // QDS 04/23, 37/23 and 04/46 are left out, qheat already registers them and
        // registerDriver reports an error for a combination claimed by two drivers.
        di.addDetection(MANUFACTURER_QDS, 0x04,  0x3e);
        di.addDetection(MANUFACTURER_QDS, 0x37,  0x3e);
        di.addDetection(MANUFACTURER_QDS, 0x37,  0x46);
        di.addDetection(MANUFACTURER_QDS, 0x04,  0x47);
        di.addDetection(MANUFACTURER_QDS, 0x37,  0x47);
        di.addDetection(MANUFACTURER_QDS, 0x04,  0x48);
        di.addDetection(MANUFACTURER_QDS, 0x37,  0x48);
        // Telegrams with a long tpl header carry the tpl type 0x0c (heat volume at inlet)
        // instead of the dll type 0x37 (radio converter) used by the short/no header frames.
        di.addDetection(MANUFACTURER_QDS, 0x0c,  0x48);
        di.addDetection(MANUFACTURER_LSE, 0x04,  0x23);
        di.addDetection(MANUFACTURER_LSE, 0x37,  0x23);
        di.addDetection(MANUFACTURER_LSE, 0x04,  0x3e);
        di.addDetection(MANUFACTURER_LSE, 0x37,  0x3e);
        di.addDetection(MANUFACTURER_LSE, 0x04,  0x46);
        di.addDetection(MANUFACTURER_LSE, 0x37,  0x46);
        di.addDetection(MANUFACTURER_LSE, 0x04,  0x47);
        di.addDetection(MANUFACTURER_LSE, 0x37,  0x47);
        di.addDetection(MANUFACTURER_LSE, 0x04,  0x48);
        di.addDetection(MANUFACTURER_LSE, 0x37,  0x48);
        di.addDetection(MANUFACTURER_LSE, 0x0c,  0x48);
        di.usesProcessContent();

        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return std::shared_ptr<Meter>(new Driver(mi, di)); });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di) : MeterCommonImplementation(mi, di)
    {
        addStringFieldWithExtractor(
            "enhanced_id",
            "Enhanced identification number.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::EnhancedIdentification)
            );

        addStringFieldWithExtractor(
            "meter_datetime",
            "Date and time when the meter sent the telegram.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::DateTime)
            );

        addStringFieldWithExtractorAndLookup(
            "status",
            "Status and error flags",
            DEFAULT_PRINT_PROPERTIES | PrintProperty::STATUS_FIELD | PrintProperty::INCLUDE_TPL_STATUS,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::ErrorFlags),
            {
                {
                    {
                        /* !!! The status bits are uncertain !!!
                           The documentation links to some intranet site, Qundis Error Codes Specification v1.7
                           at https://base/svn/sys/Meter/Gen/G5-5/PUBL/Gen55_SysSpec_Error-Codes_EN_v1.6_any.pdf
                           Here is a table that might apply:
                           https://www.manualslib.com/manual/2046543/Qundis-Q-Heat-5-5-Us.html?page=5  */
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xffff),
                        "OK",
                        {
                            { 0x0001, "NO_FLOW" },                    // F0
                            { 0x0002, "SUPPLY_SENSOR_INTERRUPTED" },  // F1
                            { 0x0004, "RETURN_SENSOR_INTERRUPTED" },  // F2
                            { 0x0008, "TEMPERATURE_ELECTRONICS_ERROR" }, // F3
                            { 0x0010, "BATTERY_VOLTAGE_ERROR" },      // F4
                            { 0x0020, "SHORT_CIRCUIT_SUPPLY_SENSOR" },// F5
                            { 0x0040, "SHORT_CIRCUIT_RETURN_SENSOR" },// F6
                            { 0x0080, "MEMORY_ERROR" },               // F7
                            // F8 - F1,2,3,5,6 longer than 8 hours, latching error, no more measurements performed.
                            { 0x0100, "SABOTAGE" },
                            { 0x0200, "ELECTRONICS_ERROR" },          // F9
                        }
                    },
                },
            });

        addNumericFieldWithExtractor(
            "total",
            "The total energy consumption recorded by this meter.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::AnyEnergyVIF)
            );

        addNumericFieldWithExtractor(
            "target",
            "The energy consumption recorded at the end of the previous month.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::AnyEnergyVIF)
            .set(StorageNr(17))
            );

        addNumericFieldWithExtractor(
            "target",
            "The end of the previous month.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::PointInTime,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Date)
            .set(StorageNr(17)),
            Unit::DateLT
            );

        addNumericFieldWithExtractor(
            "target_year",
            "The energy consumption recorded at the end of the previous year.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::AnyEnergyVIF)
            .set(StorageNr(1))
            );

        addNumericFieldWithExtractor(
            "target_year",
            "The end of the previous year.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::PointInTime,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Date)
            .set(StorageNr(1)),
            Unit::DateLT
            );

        addNumericFieldWithExtractor(
            "device_error",
            "The date when the device was in error.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::PointInTime,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::AtError)
            .set(VIFRange::Date),
            Unit::DateLT
            );

        addNumericFieldWithExtractor(
            "energy_delta_{storage_counter}",
            "Monthly energy delta.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::AnyEnergyVIF)
            .set(StorageNr(2),StorageNr(13))
            );
    }

    // Same as qdsExtractWalkByField, but the synthetic dv entry carries the storage
    // number encoded in the dif/dife key, which the field name pattern needs.
    void extractWalkByAtStorage(Telegram *t, Meter *driver, DVEntry &mfct_entry,
                                int pos, int n, const std::string &key_s,
                                const std::string &field_name, Quantity quantity,
                                StorageNr storage)
    {
        std::string bytes = mfct_entry.value.substr(pos, n);

        DifVifKey key(key_s);
        DVEntry field_entry(0, key, MeasurementType::Instantaneous, key.vif(),
                            std::set<VIFCombinable>(), std::set<uint16_t>(),
                            storage, AnyTariffNr, SubUnitNr(0), bytes);

        FieldInfo *field_info = driver->findFieldInfo(field_name, quantity);
        if (field_info == nullptr)
        {
            error("(qds) field info not found: %s\n", field_name.c_str());
            return;
        }

        field_info->performExtraction(driver, t, &field_entry);
        std::string info =
            "*** " + bytes + " (" + field_info->renderJson(driver, &field_entry) + ")";

        t->addSpecialExplanation(mfct_entry.offset + pos / 2, n / 2,
                                 KindOfData::CONTENT, Understanding::FULL,
                                 info.c_str());
    }

    // The Siemens/Qundis WalkByDataSet block: DIF=0D (variable-len), VIF=FF, VIFE=5F, 53 bytes.
    // 9-byte proprietary header (bytes 0..8):
    //   [0]   0x00     reserved
    //   [1]   0x82     wbdsId: Type 2 WalkByDataSet
    //   [2]   counter  rolling transmission counter (increments each send)
    //   [3]   0x00     reserved
    //   [4]   0x00     reserved
    //   [5]   flags    device-specific status/flags (format unknown)
    //   [6]   0x00/01  extra flag
    //   [7]   0x07     constant (possibly number of data records)
    //   [8]   0xB0=HCA, 0xC0=heat, 0xC1=warm/cold water
    // Byte [9] is the VIF (05=100Wh, 06=1kWh, 0D=100kJ, 0E=1MJ) which selects the
    // scaling of every energy value in the rest of the block. Then follow, in order:
    // an error date (which the meter always leaves as ffff), the total, the end of
    // the previous year plus its reading, the end of the previous month plus its
    // reading, twelve monthly energy deltas and one pad byte.
    void Driver::processContent(Telegram *t)
    {
        auto it = t->dv_entries.find("0DFF5F");
        if (it == t->dv_entries.end()) return;

        DVEntry entry = it->second.second;
        if (entry.value.length() != 53*2) return;

        std::string header = entry.value.substr(0, 20);
        std::transform(header.begin(), header.end(), header.begin(), ::toupper);

        // Fail closed unless the fixed header bytes [0,1], [3,4] and [7,8] match, as
        // upstream does. An encrypted block has [4]=0x35 and would give random values.
        if (header.substr(0, 4) != "0082" ||
            header.substr(6, 4) != "0000" ||
            header.substr(14, 4) != "07C0") return;

        std::string vif = header.substr(18, 2);
        if (vif != "05" && vif != "06" && vif != "0D" && vif != "0E") return;

        qdsExtractWalkByField(t, this, entry, 24, 8, "0C"+vif, "total", Quantity::Energy);
        qdsExtractWalkByField(t, this, entry, 32, 4, "426C", "target_year", Quantity::PointInTime);
        qdsExtractWalkByField(t, this, entry, 36, 8, "4C"+vif, "target_year", Quantity::Energy);
        qdsExtractWalkByField(t, this, entry, 44, 4, "C2086C", "target", Quantity::PointInTime);
        qdsExtractWalkByField(t, this, entry, 48, 8, "CC08"+vif, "target", Quantity::Energy);

        // Twelve monthly deltas, storage nr 2..13, dif/dife pairs 8201/C201 .. 8206/C206.
        static const char *delta_difs[12] = {
            "8201", "C201", "8202", "C202", "8203", "C203",
            "8204", "C204", "8205", "C205", "8206", "C206"
        };
        for (int i = 0; i < 12; i++)
        {
            extractWalkByAtStorage(t, this, entry, 56 + i*4, 4,
                                   std::string(delta_difs[i])+vif,
                                   "energy_delta_{storage_counter}", Quantity::Energy,
                                   StorageNr(2+i));
        }
    }
}

// NOTE: wmbusmeters renders an unset date (the 0xffff "invalid date" the meter
// sends) as json null. This fork's FieldInfo::extractNumeric does not check the
// return value of extractDate, so such a date comes out as "2128-03-31" instead.
// The expectations below record what this fork actually produces; the existing
// gwfwater and qwater test vectors document the same behaviour.

// Test: QHeato qheatv2 67228058 NOKEY
// telegram=|3C449344957002372337725880226793442304DC0000200C05043900004C0500000000426C9F2CCC080551070000C2086CBE29326CFFFF046D280DB62A|
// {"_": "telegram","device_error_date": "2128-03-31","id": "67228058","media": "heat","meter": "qheatv2","meter_datetime": "2021-10-22 13:40","name": "QHeato","status": "OK","target_date": "2021-09-30","target_kwh": 75.1,"target_year_date": "2020-12-31","target_year_kwh": 0,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 390.4}
// |QHeato;67228058;OK;390.4;1111-11-11 11:11.11

// Test: QQ1 qheatv2 68204641 NOKEY
// telegram=|58449344414620684737780779414620689344470c0dff5f3500825a00000e0007c00dffff310803001f3c036800003e34310803000080008000800080008000800000931a92293128190c00002f02fd170000046d000f3235|
// {"_": "telegram","energy_delta_10_kwh":295.611111,"energy_delta_11_kwh":285.805556,"energy_delta_12_kwh":86.027778,"energy_delta_13_kwh":0,"energy_delta_2_kwh":-910.222222,"energy_delta_3_kwh":-910.222222,"energy_delta_4_kwh":-910.222222,"energy_delta_5_kwh":-910.222222,"energy_delta_6_kwh":-910.222222,"energy_delta_7_kwh":-910.222222,"energy_delta_8_kwh":0,"energy_delta_9_kwh":188.972222,"enhanced_id": "0C47449368204641","id": "68204641","media": "radio converter (meter side)","meter": "qheatv2","meter_datetime": "2025-05-18 15:00","name": "QQ1","status": "OK","target_date": "2025-04-30","target_kwh":856.416667,"target_year_date": "2024-12-31","target_year_kwh":188.972222,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 856.416667}
// |QQ1;68204641;OK;856.416667;1111-11-11 11:11.11

// Test: QHeat55r qheatv2 68267443 NOKEY
// Comment: Q AMR telegram sent every 7.5 minutes
// telegram=|414493444374266848377243742668934448042A000020_0C06140000004C0600000000426CFFFFCC080600000000C2086C3C3202FD170000326CFFFF046D190F3533|
// {"_": "telegram","device_error_date": "2128-03-31","id": "68267443","media": "heat","meter": "qheatv2","meter_datetime": "2025-03-21 15:25","name": "QHeat55r","status": "OK","target_date": "2025-02-28","target_kwh": 0,"target_year_date": "2128-03-31","target_year_kwh": 0,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 14}
// |QHeat55r;68267443;OK;14;1111-11-11 11:11.11

// Test: QHeat55r qheatv2 68267443 NOKEY
// Comment: Q Walk by telegram sent every 112 seconds
// telegram=|5844934443742668483778_077943742668934448040DFF5F3500822A0000610107C006FFFF14000000FFFF000000003C32000000000080008000800080008000800080008000800080008000002F02FD170000046D1B0F3533|
// {"_": "telegram","energy_delta_10_kwh":-32768,"energy_delta_11_kwh":-32768,"energy_delta_12_kwh":-32768,"energy_delta_13_kwh":0,"energy_delta_2_kwh":-32768,"energy_delta_3_kwh":-32768,"energy_delta_4_kwh":-32768,"energy_delta_5_kwh":-32768,"energy_delta_6_kwh":-32768,"energy_delta_7_kwh":-32768,"energy_delta_8_kwh":-32768,"energy_delta_9_kwh":-32768,"enhanced_id": "0448449368267443","id": "68267443","media": "radio converter (meter side)","meter": "qheatv2","meter_datetime": "2025-03-21 15:27","name": "QHeat55r","status": "OK","target_date": "2025-02-28","target_kwh": 0,"target_year_date": "2128-03-31","target_year_kwh": 0,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 14}
// |QHeat55r;68267443;OK;14;1111-11-11 11:11.11

// Test: QQA qheatv2 67958979 NOKEY
// telegram=|58449344798995673E377807797989956793443E040DFF5F3500827D0000810007C006FFFF531100003F3C490900005F318010000083003C0022000B0000000000000000000A002B00510083002F02FD170000046D16085832|
// {"_": "telegram","energy_delta_10_kwh":10,"energy_delta_11_kwh":43,"energy_delta_12_kwh":81,"energy_delta_13_kwh":131,"energy_delta_2_kwh":131,"energy_delta_3_kwh":60,"energy_delta_4_kwh":34,"energy_delta_5_kwh":11,"energy_delta_6_kwh":0,"energy_delta_7_kwh":0,"energy_delta_8_kwh":0,"energy_delta_9_kwh":0,"enhanced_id": "043E449367958979","id": "67958979","media": "radio converter (meter side)","meter": "qheatv2","meter_datetime": "2026-02-24 08:22","name": "QQA","status": "OK","target_date": "2026-01-31","target_kwh":1080,"target_year_date": "2025-12-31","target_year_kwh":949,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 1153}
// |QQA;67958979;OK;1153;1111-11-11 11:11.11

// Test: QQB qheatv2 67985890 NOKEY
// telegram=|41449344905898673e37729058986793443e04120000200c06809300004c0678820000426c3f3ccc080636900000c2086c5f3102fd170000326cffff046d1d095832|
// {"_": "telegram","device_error_date": "2128-03-31","id": "67985890","media": "heat","meter": "qheatv2","meter_datetime": "2026-02-24 09:29","name": "QQB","status": "OK","target_date": "2026-01-31","target_kwh": 9036,"target_year_date": "2025-12-31","target_year_kwh": 8278,"timestamp": "1111-11-11T11:11:11Z","total_kwh": 9380}
// |QQB;67985890;OK;9380;1111-11-11 11:11.11

// Test: QQC qheatv2 11223344 NOKEY
// Comment: A Q heat 5.5 behind a radio converter, the long tpl header announces media 0x0c heat volume at inlet.
// telegram=|4144934444332211483772443322119344480cce0000200c0d100604004c0d00000000426cffffcc080d29120100c2086c5f3702fd170000326cffff046d2a105538|
// {"_": "telegram","device_error_date": "2128-03-31","id": "11223344","media": "heat volume at inlet","meter": "qheatv2","meter_datetime": "2026-08-21 16:42","name": "QQC","status": "OK","target_date": "2026-07-31","target_kwh":311.916667,"target_year_date": "2128-03-31","target_year_kwh": 0,"timestamp": "1111-11-11T11:11:11Z","total_kwh":1128.055556}
// |QQC;11223344;OK;1128.055556;1111-11-11 11:11.11


KEEP_DRIVER(qheatv2);
