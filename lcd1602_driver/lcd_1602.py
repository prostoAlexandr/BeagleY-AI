import time
from smbus2 import SMBus

# Константы сигналов PCF8574T -> HD44780
LCD_RS = 0x01        # P0 -> RS (0: команда, 1: данные)
LCD_RW = 0x02        # P1 -> RW (заземлен / не используется)
LCD_EN = 0x04        # P2 -> Enable
LCD_BACKLIGHT = 0x08 # P3 -> Управление подсветкой (1: включена)

# Команды дисплея
LCD_CLEAR = 0x01
LCD_ENTRY_MODE = 0x06
LCD_DISPLAY_ON = 0x0C
LCD_FUNCTION_SET = 0x28 # 4-битный режим, 2 строки, шрифт 5x8


class LCD1602:
    def __init__(self, bus_num: int = 1, addr: int = 0x27):
        self.bus_num = bus_num
        self.addr = addr
        self.bus = SMBus(self.bus_num)
        self._init_lcd()

    def _write_byte(self, data: int) -> None:
        self.bus.write_byte(self.addr, data)

    def _pulse_enable(self, data: int) -> None:
        self._write_byte(data | LCD_EN)
        time.sleep(0.0005)
        self._write_byte(data & ~LCD_EN)
        time.sleep(0.0001)

    def _send_nibble(self, nibble: int, mode: int = 0) -> None:
        # Передача старших 4 бит на пины P4-P7 расширителя
        data = (nibble & 0xF0) | mode | LCD_BACKLIGHT
        self._write_byte(data)
        self._pulse_enable(data)

    def send_command(self, cmd: int) -> None:
        self._send_nibble(cmd & 0xF0, mode=0)
        self._send_nibble((cmd << 4) & 0xF0, mode=0)

    def send_data(self, data: int) -> None:
        self._send_nibble(data & 0xF0, mode=LCD_RS)
        self._send_nibble((data << 4) & 0xF0, mode=LCD_RS)

    def _init_lcd(self) -> None:
        time.sleep(0.05)
        # Сброс и сведение контроллера в 4-битный режим
        self._send_nibble(0x30)
        time.sleep(0.005)
        self._send_nibble(0x30)
        time.sleep(0.001)
        self._send_nibble(0x30)
        self._send_nibble(0x20)

        # Конфигурация параметров экрана
        self.send_command(LCD_FUNCTION_SET)
        self.send_command(LCD_DISPLAY_ON)
        self.send_command(LCD_CLEAR)
        self.send_command(LCD_ENTRY_MODE)
        time.sleep(0.002)

    def clear(self) -> None:
        self.send_command(LCD_CLEAR)
        time.sleep(0.002)

    def print_line(self, text: str, line: int = 1) -> None:
        # 0x80 — первая строка, 0xC0 — вторая строка
        addr = 0x80 if line == 1 else 0xC0
        self.send_command(addr)
        # Обрезка до 16 символов и дополнение пробелами
        formatted_text = text[:16].ljust(16)
        for char in formatted_text:
            self.send_data(ord(char))


if __name__ == "__main__":
    # Замените bus_num и addr на значения вашей системы
    lcd = LCD1602(bus_num=1, addr=0x27)

    lcd.clear()
    lcd.print_line("BeagleY-AI Ready", line=1)
    lcd.print_line("Debian IoT / I2C", line=2)
