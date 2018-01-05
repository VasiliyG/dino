module Dino
  class Board
    attr_reader :high, :low, :components, :analog_zero, :dac_zero
    DIVIDERS = [1, 2, 4, 8, 16, 32, 64, 128]

    def initialize(io, options={})
      @io, @components = io, []
      io.add_observer(self)

      @analog_zero, @dac_zero = @io.handshake.to_s.split(",").map { |pin| pin.to_i }
      self.analog_resolution = options[:bits]
      start_read
    end

    def analog_resolution=(value)
      @bits = value || 8
      write Dino::Message.encode(command: 96, value: @bits)
      @low  = 0
      @high = (2 ** @bits) - 1
    end

    def analog_divider=(value)
      unless DIVIDERS.include? value
        puts "Analog divider must be in #{DIVIDERS.inspect}"
      else
        write Dino::Message.encode(command: 97, value: value)
      end
    end

    def heart_rate=(value)
      write Dino::Message.encode(command: 98, aux_message: value)
    end

    def start_read
      @io.read
    end

    def stop_read
      @io.close_read
    end

    def write(msg)
      @io.write(msg)
    end

    def update(pin, msg)
      @components.each do |part|
        part.update(msg) if pin.to_i == part.pin
      end
    end

    def add_component(component)
      @components << component
    end

    def remove_component(component)
      stop_listener(component.pin)
      @components.delete(component)
    end

    def set_pin_mode(pin, mode)
      pin, value = convert_pin(pin), mode == :out ? 0 : 1
      write Dino::Message.encode(command: 0, pin: pin, value: value)
    end

    def set_pullup(pin, pullup)
      pin = convert_pin(pin)
      pullup ? digital_write(pin, @high) : digital_write(pin, @low)
    end

    PIN_COMMANDS = {
      # set_pin_mode:  '0'
      digital_write:   '1',
      digital_read:    '2',
      analog_write:    '3',
      analog_read:     '4',
      digital_listen:  '5',
      analog_listen:   '6',
      stop_listener:   '7',
      servo_toggle:    '8',
      servo_write:     '9',
      # LCD            '10'
      # Unused         '11'
      # SoftSerial     '12'
      dht_read:        '13',
      # HCSR04         '14'
      ds18b20_read:    '15',
      # IR send:       '16'
      tone:            '17',
      no_tone:         '18'
    }

    PIN_COMMANDS.each_key do |command|
      define_method(command) do |pin, value=nil|
        write Dino::Message.encode(command: PIN_COMMANDS[command], pin: convert_pin(pin), value: value)
      end
    end

    # Redefinition tone to accept duration.
    def tone(pin, value, duration)
      write Dino::Message.encode(command: PIN_COMMANDS[:tone], pin: convert_pin(pin), value: value, aux_message: duration)
    end

    DIGITAL_REGEX = /\A\d+\z/i
    ANALOG_REGEX = /\A(a)\d+\z/i
    DAC_REGEX = /\A(dac)\d+\z/i

    def convert_pin(pin)
      pin = pin.to_s
      return pin.to_i             if pin.match(DIGITAL_REGEX)
      return analog_pin_to_i(pin) if pin.match(ANALOG_REGEX)
      return dac_pin_to_i(pin)    if pin.match(DAC_REGEX)
      raise "Incorrect pin format"
    end

    def analog_pin_to_i(pin)
      @analog_zero + pin.gsub(/\Aa/i, '').to_i
    end

    def dac_pin_to_i(pin)
      raise "The board did not specify any DAC pins" unless @dac_zero
      @dac_zero + pin.gsub(/\Adac/i, '').to_i
    end
  end
end
