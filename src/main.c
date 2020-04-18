/* A portunun Pin_0 bagli potansiyometreden ADC1 cevresel birimiyle  veriyi alip cpu yu kullanmadan dogrudan bellege yaziyoruz */
#include "stm32f4xx.h"
#include "stm32f4_discovery.h"

#define BufferLength 1

uint16_t adc_value[BufferLength];

void GPIO_Config() {
	GPIO_InitTypeDef GPIO_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AN; 
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOA,&GPIO_InitStruct);
}

void ADC_Config() {
	ADC_InitTypeDef ADC_InitStruct;
	ADC_CommonInitTypeDef ADC_CommonStruct;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);

	ADC_CommonStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonStruct.ADC_Prescaler = ADC_Prescaler_Div4; // ADC max 36 Mhz degerinde calisir ancak APB2 Clock hatti 84Mhz oldugu icin bolme yapmaliyiz
	ADC_CommonStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;	//Tek kanaldan DMA okumasi yaptigimiz icin disable dedik.cok kanalli olsa idi diger modlardan birini sececektik
	ADC_CommonStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_20Cycles;	//iki ornekleme arasindaki bekleme suresini sectik.datalar birbirine girmemesi icin

	ADC_Init(ADC1 , &ADC_CommonStruct);

	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode = ENABLE;	    // COKLU ADC OKUMALARİNDA TARAMA YAPMAK icin kullanilir.burada coklu okuma yapmiyoruz aslinda
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // ADC okumasinda harici tetikleme ile baslatma saglar.tetikleme yok dedik
	ADC_InitStruct.ADC_ExternalTrigConv = 0;	    // Cevrim tetigi yok dedik
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right; // 16 bitlik bir register e yazma yapildigi icin saga - sola yasli yazilma durumunu belirledik
	ADC_InitStruct.ADC_ContinuousConvMode = ENABLE;     // ADC cevriminin surekli yapilmasini istiyoruz.Disable yaparsak tek bir sefer cevrim yapar
	ADC_InitStruct.ADC_NbrOfConversion = BufferLength;  // Bir adet ADC cevrim okumasi yapacagimizi belirtik.ADC1 kullanildi sadece

	ADC_Init(ADC1 , &ADC_InitStruct);
	ADC_Cmd(ADC1,ENABLE); // ADC cevresel birim

	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_3Cycles);
	ADC_DMARequestAfterLastTransferCmd(ADC1,ENABLE); // her ADC DMA transferinden sonra yeni transferler icin istek yapiyoruz.Bu mutlaka yapilmali
	ADC_DMACmd(ADC1 , ENABLE); // ADC yi DMA ile bagladik
}

void DMA_Config()
{
	DMA_InitTypeDef DMA_InitStruct;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);

	DMA_InitStruct.DMA_Channel = DMA_Channel_0;		// syf 308
	DMA_InitStruct.DMA_Priority = DMA_Priority_VeryHigh ; 	// oncelik durumu
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t ) & ADC1 -> DR; // DMA YA veriyi ADC1 İN DATE REGİSTER(DR) den alacagini soyledik
	/* ADC regular data register (ADC_DR)(425) ADC de yapilan cevrimleri icerisinde tutar.Yani DMA ile verileri buradan alip RAM e gonderiyoruz. */
	DMA_InitStruct.DMA_Memory0BaseAddr = (uint32_t) & adc_value[1]; // ADC den okudugumuz veriyi adc_value degisken adresine yaziyoruz.
	DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralToMemory; // DMA islemini cevresel birimden memory ye yapacak.
	DMA_InitStruct.DMA_BufferSize = BufferLength;	   // kac farklı cevrim yapacagimizi belirttik
	DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Enable; // ilk giren veri ilk cıksın dedik.
	DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;//fifo verileri ne kadar doluyken iletsin
	DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Circular; // hic durmadan veri aktarimina devam etsin ADC den MEMORY e
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord; // 16 bit sectik cunki 12 bitlik bir adc okumasi yapıyoruz
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // yukaridakiyle aynı olmak zorunda
	DMA_InitStruct.DMA_MemoryInc = ENABLE; 	    // hafizaya yazma işleminde adresin surekli degismesini saglıyoruz.yan yana yazildigi icin adresin surekli degismesi gerekmektedir.
	DMA_InitStruct.DMA_PeripheralInc = DISABLE; // ancak veri okudugumuz adresin surekli sabit kalmasini sagliyoruz.
	
	// DMA1 ADC islemlerini desteklemez bu yuzden ADC calismalarinda DMA2 kullanilmalidir.
	DMA_Init(DMA2_Stream0 , &DMA_InitStruct);
	DMA_Cmd(DMA2_Stream0,ENABLE); // DMA cevresel birimdir.
}

int main(void) {
	GPIO_Config();
	ADC_Config();
	DMA_Config();

	ADC_SoftwareStartConv(ADC1); // ADC yi yazilimsal olarak baslatiyoruz.
	/* ADC_SoftwareStartConv islemini while icinde yapmadık cunku while icinde yaparsak cevresel birimden -> CPU -> RAM seklinde
	 * yazmıs oluruz. Ama burada direk olarak cevresel birim -> RAM  seklinde bir yazma gerceklestirdik.  */
  while (1) {

  }
}

void EVAL_AUDIO_TransferComplete_CallBack(uint32_t pBuffer, uint32_t Size){
  /* TODO, implement your code here */
  return;
}

/*
 * Callback used by stm324xg_eval_audio_codec.c.
 * Refer to stm324xg_eval_audio_codec.h for more info.
 */
uint16_t EVAL_AUDIO_GetSampleCallBack(void){
  /* TODO, implement your code here */
  return -1;
}
