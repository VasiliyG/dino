require 'socket'

module Dino
  module TxRx
    class TCP < Base
      def initialize(host, port=3466)
        @host, @port = host, port
      end

    private

      def connect
        Timeout::timeout(10) { TCPSocket.open(@host, @port) }
      rescue
        raise BoardNotFound
      end

      def io_write(message)
        loop do
          if IO.select(nil, [io], nil)
            io.syswrite(message)
            break
          end
        end
      end

      def gets(timeout=0.005)
        IO.select([io], nil, nil, timeout) && io.gets.gsub(/\n\z/, "")
      end
    end
  end
end
