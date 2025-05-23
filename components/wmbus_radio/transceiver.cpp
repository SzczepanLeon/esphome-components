#include "transceiver.h"

#include "esphome/core/log.h"

#include "freertos/FreeRTOS.h"

namespace esphome
{
    namespace wmbus_radio
    {
        static const char *TAG = "wmbus.transceiver";

        bool RadioTransceiver::read_in_task(uint8_t *buffer, size_t length)
        {
            const uint8_t *buffer_end = buffer + length;
            int wait_count = 0;

            while (buffer != buffer_end)
            {
                auto byte = this->read();
                if (byte.has_value())
                    *buffer++ = *byte;
                else if (!ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)))
                    return false;
                else
                    wait_count++;
            }

            return true;
        }

        void RadioTransceiver::set_reset_pin(InternalGPIOPin *reset_pin)
        {
            this->reset_pin_ = reset_pin;
        }

        void RadioTransceiver::set_irq_pin(InternalGPIOPin *irq_pin)
        {
            this->irq_pin_ = irq_pin;
        }

        void RadioTransceiver::reset()
        {
            this->reset_pin_->digital_write(0);
            delay(5);
            this->reset_pin_->digital_write(1);
            delay(5);
        }

        void RadioTransceiver::common_setup()
        {
            this->reset_pin_->setup();
            this->irq_pin_->setup();
            this->spi_setup();
        }

        uint8_t RadioTransceiver::spi_transaction(uint8_t operation, uint8_t address, std::initializer_list<uint8_t> data)
        {
            this->delegate_->begin_transaction();
            auto rval = this->delegate_->transfer(operation | address);
            for (auto byte : data)
                rval = this->delegate_->transfer(byte);
            this->delegate_->end_transaction();
            return rval;
        }

        uint8_t RadioTransceiver::spi_read(uint8_t address)
        {
            return this->spi_transaction(0x00, address, {0});
        }

        void RadioTransceiver::spi_write(uint8_t address, std::initializer_list<uint8_t> data)
        {
            this->spi_transaction(0x80, address, data);
        }

        void RadioTransceiver::spi_write(uint8_t address, uint8_t data)
        {
            this->spi_write(address, {data});
        }

        void RadioTransceiver::dump_config()
        {
            ESP_LOGCONFIG(TAG, "Transceiver: %s", this->get_name());
            LOG_PIN("Reset Pin: ", this->reset_pin_);
            LOG_PIN("IRQ Pin: ", this->irq_pin_);
        }
    }
}
