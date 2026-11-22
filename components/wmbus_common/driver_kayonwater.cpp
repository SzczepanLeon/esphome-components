/*
 Copyright (C) 2026 Igor K. (gpl-3.0-or-later)

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

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);
    };

    static bool ok = registerDriver([](DriverInfo&di)
    {
        di.setName("kayonwater");
        di.setDefaultFields("name,id,total_m3,status,timestamp");
        di.setMeterType(MeterType::WaterMeter);
        di.addDetection(MANUFACTURER_KYN, 0x07,  0x01);

        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return std::shared_ptr<Meter>(new Driver(mi, di)); });
    });

    /* The KYN telegrams are produced by Kayon's own wM-Bus add-on radio modules
       (Modularis One for Modularis-mount meters, WMBus Smart+ for Apator Powogaz
       Smart+/Smart C+), not by the meter body itself. Per their datasheets the
       modules emit three configurable frame types:

         short frame - volume, reverse flow, alarms, serial, date/time, last month
         long frame  - short frame plus 1..15 months of readings and of alarms
         long+ frame - long frame plus 1..15 months of reverse flow

       which is why every history range below spans the full 1..15 months even
       though a meter only sends as many months as it is configured for.

       The modules raise six named alarms: overflow, backflow, leak, low battery,
       magnetic field tamper and module removal. Kayon does not publish the bit
       positions, so the lookups below deliberately map no bits and report set
       bits verbatim as ERROR_FLAGS_XX.
       https://kayon.eu/en/products/communication-modules */

    Driver::Driver(MeterInfo &mi, DriverInfo &di) : MeterCommonImplementation(mi, di)
    {
        addStringFieldWithExtractor(
            "meter_datetime",
            "Date and time when the meter sent the telegram.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::DateTime)
            );

        addNumericFieldWithExtractor(
            "total",
            "The total media volume consumption recorded by this meter.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Volume)
            );

        addNumericFieldWithExtractor(
            "total_backward",
            "The total media volume flowing backward.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Volume)
            .add(VIFCombinable::BackwardFlow)
            );

        addStringFieldWithExtractorAndLookup(
            "status",
            "Meter status from error flags and tpl status field.",
            DEFAULT_PRINT_PROPERTIES | PrintProperty::STATUS_FIELD | PrintProperty::INCLUDE_TPL_STATUS,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::ErrorFlags),
            {
                {
                    {
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xff),
                        "OK",
                        {
                        }
                    },
                },
            });

        addNumericFieldWithExtractor(
            "consumption_at_set_date_{storage_counter}",
            "Water consumption at the end of the # billing period, ie month.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Volume)
            .set(StorageNr(1),StorageNr(15))
            );

        addNumericFieldWithExtractor(
            "backward_consumption_at_set_date_{storage_counter}",
            "Backward flowing water volume at the end of the # billing period, ie month.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Volume,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::Volume)
            .add(VIFCombinable::BackwardFlow)
            .set(StorageNr(1),StorageNr(15))
            );

        addStringFieldWithExtractorAndLookup(
            "status_at_set_date_{storage_counter}",
            "Meter status at the end of the # billing period, ie month.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::ErrorFlags)
            .set(StorageNr(1),StorageNr(15)),
            {
                {
                    {
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xff),
                        "OK",
                        {
                        }
                    },
                },
            });
    }
}

// The trailing 03FF00 manufacturer specific record is not yet decoded.
// Test: Water1 kayonwater 12345678 NOKEY
// telegram=|ca442e2f7856341201077ab60000a004130300000004933c00000000046da52f553801fd174041fd17028101fd1702c101fd17008102fd1700c102fd17008103fd170044130000000084011300000000c401130000000084021300000000c402130000000084031300000000c403130000000084041300000000c404130000000084051300000000c40513000000008406130000000044933c000000008401933c00000000c401933c000000008402933c00000000c402933c000000008403933c0000000003ff00004a00|
// {"_":"telegram","media":"water","meter":"kayonwater","name":"Water1","id":"12345678","backward_consumption_at_set_date_1_m3":0,"backward_consumption_at_set_date_2_m3":0,"backward_consumption_at_set_date_3_m3":0,"backward_consumption_at_set_date_4_m3":0,"backward_consumption_at_set_date_5_m3":0,"backward_consumption_at_set_date_6_m3":0,"consumption_at_set_date_1_m3":0,"consumption_at_set_date_10_m3":0,"consumption_at_set_date_11_m3":0,"consumption_at_set_date_12_m3":0,"consumption_at_set_date_2_m3":0,"consumption_at_set_date_3_m3":0,"consumption_at_set_date_4_m3":0,"consumption_at_set_date_5_m3":0,"consumption_at_set_date_6_m3":0,"consumption_at_set_date_7_m3":0,"consumption_at_set_date_8_m3":0,"consumption_at_set_date_9_m3":0,"total_m3":0.003,"total_backward_m3":0,"meter_datetime":"2026-08-21 15:37","status":"ERROR_FLAGS_40","status_at_set_date_1":"ERROR_FLAGS_2","status_at_set_date_2":"ERROR_FLAGS_2","status_at_set_date_3":"OK","status_at_set_date_4":"OK","status_at_set_date_5":"OK","status_at_set_date_6":"OK","timestamp":"1111-11-11T11:11:11Z"}
// |Water1;12345678;0.003;ERROR_FLAGS_40;1111-11-11 11:11.11

// Test: Water2 kayonwater 87654321 NOKEY
// telegram=|ca442e2f2143658701077a1b0000a004130000000004933c00000000046da72f553801fd174041fd17028101fd1702c101fd17008102fd1700c102fd17008103fd170044130000000084011300000000c401130000000084021300000000c402130000000084031300000000c403130000000084041300000000c404130000000084051300000000c40513000000008406130000000044933c000000008401933c00000000c401933c000000008402933c00000000c402933c000000008403933c0000000003ff00004b00|
// {"_":"telegram","media":"water","meter":"kayonwater","name":"Water2","id":"87654321","backward_consumption_at_set_date_1_m3":0,"backward_consumption_at_set_date_2_m3":0,"backward_consumption_at_set_date_3_m3":0,"backward_consumption_at_set_date_4_m3":0,"backward_consumption_at_set_date_5_m3":0,"backward_consumption_at_set_date_6_m3":0,"consumption_at_set_date_1_m3":0,"consumption_at_set_date_10_m3":0,"consumption_at_set_date_11_m3":0,"consumption_at_set_date_12_m3":0,"consumption_at_set_date_2_m3":0,"consumption_at_set_date_3_m3":0,"consumption_at_set_date_4_m3":0,"consumption_at_set_date_5_m3":0,"consumption_at_set_date_6_m3":0,"consumption_at_set_date_7_m3":0,"consumption_at_set_date_8_m3":0,"consumption_at_set_date_9_m3":0,"total_m3":0,"total_backward_m3":0,"meter_datetime":"2026-08-21 15:39","status":"ERROR_FLAGS_40","status_at_set_date_1":"ERROR_FLAGS_2","status_at_set_date_2":"ERROR_FLAGS_2","status_at_set_date_3":"OK","status_at_set_date_4":"OK","status_at_set_date_5":"OK","status_at_set_date_6":"OK","timestamp":"1111-11-11T11:11:11Z"}
// |Water2;87654321;0;ERROR_FLAGS_40;1111-11-11 11:11.11


KEEP_DRIVER(kayonwater);
