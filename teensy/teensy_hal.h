
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0 : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0)
#endif /* USE_FULL_ASSERT */    

#define FTM0    ((FTM_TypeDef *)&FTM0_SC)
#define FTM1    ((FTM_TypeDef *)&FTM1_SC)
#define FTM2    ((FTM_TypeDef *)&FTM2_SC)

#define GPIOA   ((GPIO_TypeDef *)&GPIOA_PDOR)
#define GPIOB   ((GPIO_TypeDef *)&GPIOB_PDOR)
#define GPIOC   ((GPIO_TypeDef *)&GPIOC_PDOR)
#define GPIOD   ((GPIO_TypeDef *)&GPIOD_PDOR)
#define GPIOE   ((GPIO_TypeDef *)&GPIOE_PDOR)
#define GPIOZ   ((GPIO_TypeDef *)NULL)

#define I2C0    ((I2C_TypeDef *)0x40066000)
#define I2C1    ((I2C_TypeDef *)0x40067000)

#undef  SPI0
#define SPI0    ((SPI_TypeDef *)0x4002C000)
#define SPI1    ((SPI_TypeDef *)0x4002D000)

#define UART0   ((UART_TypeDef *)&UART0_BDH)
#define UART1   ((UART_TypeDef *)&UART1_BDH)
#define UART2   ((UART_TypeDef *)&UART2_BDH)

typedef struct {
    uint32_t dummy;
} FTM_TypeDef;

typedef struct {
    uint32_t dummy;
} I2C_TypeDef;

typedef struct {
    uint32_t dummy;
} UART_TypeDef;

typedef struct {
    uint32_t dummy;
} SPI_TypeDef;

typedef struct {
    volatile    uint32_t    PDOR;   // Output register
    volatile    uint32_t    PSOR;   // Set output register
    volatile    uint32_t    PCOR;   // Clear output register
    volatile    uint32_t    PTOR;   // Toggle output register
    volatile    uint32_t    PDIR;   // Data Input register
    volatile    uint32_t    PDDR;   // Data Direction register
} GPIO_TypeDef;

#define GPIO_OUTPUT_TYPE    ((uint32_t)0x00000010)  // Indicates OD

#define GPIO_MODE_INPUT     ((uint32_t)0x00000000)
#define GPIO_MODE_OUTPUT_PP ((uint32_t)0x00000001)
#define GPIO_MODE_OUTPUT_OD ((uint32_t)0x00000011)
#define GPIO_MODE_AF_PP     ((uint32_t)0x00000002)
#define GPIO_MODE_AF_OD     ((uint32_t)0x00000012)
#define GPIO_MODE_ANALOG    ((uint32_t)0x00000003)

#define IS_GPIO_MODE(MODE) (((MODE) == GPIO_MODE_INPUT)              ||\
                            ((MODE) == GPIO_MODE_OUTPUT_PP)          ||\
                            ((MODE) == GPIO_MODE_OUTPUT_OD)          ||\
                            ((MODE) == GPIO_MODE_AF_PP)              ||\
                            ((MODE) == GPIO_MODE_AF_OD)              ||\
                            ((MODE) == GPIO_MODE_ANALOG))

#define GPIO_NOPULL         ((uint32_t)0)
#define GPIO_PULLUP         ((uint32_t)1)
#define GPIO_PULLDOWN       ((uint32_t)2)

#define IS_GPIO_PULL(PULL) (((PULL) == GPIO_NOPULL) || ((PULL) == GPIO_PULLUP) || \
                            ((PULL) == GPIO_PULLDOWN))

#define  GPIO_SPEED_LOW         ((uint32_t)0)
#define  GPIO_SPEED_MEDIUM      ((uint32_t)1)
#define  GPIO_SPEED_FAST        ((uint32_t)2)
#define  GPIO_SPEED_HIGH        ((uint32_t)3)

#define IS_GPIO_AF(af)      ((af) >= 0 && (af) <= 7)

typedef struct {
    uint32_t    Pin;
    uint32_t    Mode;
    uint32_t    Pull;
    uint32_t    Speed;
    uint32_t    Alternate;
} GPIO_InitTypeDef;

#define GPIO_PORT_TO_PORT_NUM(GPIOx) \
    ((GPIOx->PDOR - GPIOA_PDOR) / (GPIOB_PDOR - GPIOA_PDOR))

#define GPIO_PIN_TO_PORT_PCR(GPIOx, pin) \
    (&PORTA_PCR0 + GPIO_PORT_TO_PORT_NUM(GPIOx) * 32 + (pin))

#define GPIO_AF2_I2C0   2
#define GPIO_AF2_I2C1   2
#define GPIO_AF2_SPI0   2
#define GPIO_AF3_FTM0   3
#define GPIO_AF3_FTM1   3
#define GPIO_AF3_FTM2   3
#define GPIO_AF3_UART0  3
#define GPIO_AF3_UART1  3
#define GPIO_AF3_UART2  3
#define GPIO_AF4_FTM0   4
#define GPIO_AF6_FTM1   6
#define GPIO_AF6_FTM2   6
#define GPIO_AF6_I2C1   6
#define GPIO_AF7_FTM1   7


__attribute__(( always_inline )) static inline void __WFI(void)
{
  __asm volatile ("wfi");
}

uint32_t HAL_GetTick(void);
void     HAL_Delay(uint32_t Delay);

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *init);

#define GPIO_read_pin(gpio, pin)        (((gpio)->PDIR >> (pin)) & 1)
#define GPIO_set_pin(gpio, pin_mask)    (((gpio)->PSOR) = (pin_mask))
#define GPIO_clear_pin(gpio, pin_mask)  (((gpio)->PCOR) = (pin_mask))
#define GPIO_read_output_pin(gpio, pin) (((gpio)->PDOR >> (pin)) & 1)
