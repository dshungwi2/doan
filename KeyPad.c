#include "KeyPad.h"
#include "KeyPadConfig.h"
#include "stm32f1xx_hal.h"
#if (_KEYPAD_USE_FREERTOS==1)
#include "cmsis_os.h"
#define _KEYPAD_DELAY(x)      osDelay(x)
#else
#define _KEYPAD_DELAY(x)      HAL_Delay(x)
#endif

KeyPad_t	KeyPad;
const char keyMap[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
//#############################################################################################
void	KeyPad_Init(void)
{
  GPIO_InitTypeDef	gpio;
  KeyPad.ColumnSize = sizeof(_KEYPAD_COLUMN_GPIO_PIN) / 2;
  KeyPad.RowSize = sizeof(_KEYPAD_ROW_GPIO_PIN) / 2;
  for(uint8_t	i=0 ; i<KeyPad.ColumnSize ; i++)
  {
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = _KEYPAD_COLUMN_GPIO_PIN[i];
    HAL_GPIO_Init((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], &gpio);
    HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], _KEYPAD_COLUMN_GPIO_PIN[i], GPIO_PIN_SET);
  }
  for(uint8_t	i=0 ; i<KeyPad.RowSize ; i++)
  {
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Pin = _KEYPAD_ROW_GPIO_PIN[i];
    HAL_GPIO_Init((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[i], &gpio);		
  }
}
//#############################################################################################
uint16_t	KeyPad_Scan(void)
{
  uint16_t  key=0;
  for(uint8_t c=0 ; c<KeyPad.ColumnSize ; c++)
  {
    for(uint8_t i=0 ; i<KeyPad.ColumnSize ; i++)
      HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[i], _KEYPAD_COLUMN_GPIO_PIN[i], GPIO_PIN_SET);
			HAL_GPIO_WritePin((GPIO_TypeDef*)_KEYPAD_COLUMN_GPIO_PORT[c], _KEYPAD_COLUMN_GPIO_PIN[c], GPIO_PIN_RESET);
    _KEYPAD_DELAY(5);
    for(uint8_t r=0 ; r<KeyPad.RowSize ; r++)
    {
      if(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
      {
        _KEYPAD_DELAY(_KEYPAD_DEBOUNCE_TIME_MS);
        if(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
        {
          key |= 1<<c;					
          key |= 1<<(r+8);
          while(HAL_GPIO_ReadPin((GPIO_TypeDef*)_KEYPAD_ROW_GPIO_PORT[r], _KEYPAD_ROW_GPIO_PIN[r]) == GPIO_PIN_RESET)
            _KEYPAD_DELAY(5);
          return key;
        }
      }			
    }		
  }
  return key;
}
//#############################################################################################
char KeyPad_WaitForKeyGetChar()
{
    uint16_t key = KeyPad_Scan();

    if (key == 0) return 0;

    uint8_t row = 0x0F & key;         
    uint8_t col = (key >> 8) & 0x0F;   

    int colIndex = -1, rowIndex = -1;

    // Tìm v? trí c?t
    for (int i = 0; i < 4; i++) {
        if (col == (1 << i)) {
            colIndex = i;
            break;
        }
    }

    // Tìm v? trí hàng
    for (int i = 0; i < 4; i++) {
        if (row == (1 << i)) {
            rowIndex = i;
            break;
        }
    }

    if (rowIndex >= 0 && colIndex >= 0) {
        return keyMap[rowIndex][colIndex];
    }

    return 0;
}
