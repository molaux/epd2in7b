#include <node.h>
#include <iostream>
#include <functional>
#include <nan.h>

using namespace std;

#include "epdif.h"

using v8::Exception;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::Number;
using v8::Object;
using v8::String;
using v8::Boolean;
using v8::Value;
using v8::Context;
using v8::Array;

// Async worker
class Epd2In7AsyncWorker : public Nan::AsyncWorker {
  public:
    function <void ()> method;

    Epd2In7AsyncWorker(function <void ()> method, Nan::Callback *callback)
      : Nan::AsyncWorker(callback) {
      this->method = method;
    }


    void Execute() {
      this->method();
    }

    void HandleOKCallback() {
      Nan::HandleScope scope;
      v8::Local<v8::Value> argv[] = {
        Nan::Null(), // no error occured
        Nan::Null()
      };
      callback->Call(2, argv);
    }

    void HandleErrorCallback() {
      Nan::HandleScope scope;
      v8::Local<v8::Value> argv[] = {
        Nan::New(this->ErrorMessage()).ToLocalChecked(), // return error message
        Nan::Null()
      };
      callback->Call(2, argv);
    }
};

// Display resolution
#define EPD_WIDTH       176
#define EPD_HEIGHT      264

#define PANEL_SETTING                               0x00
#define POWER_SETTING                               0x01
#define POWER_OFF                                   0x02
#define POWER_OFF_SEQUENCE_SETTING                  0x03
#define POWER_ON                                    0x04
#define POWER_ON_MEASURE                            0x05
#define BOOSTER_SOFT_START                          0x06
#define DEEP_SLEEP                                  0x07
#define DATA_START_TRANSMISSION_1                   0x10
#define DATA_STOP                                   0x11
#define DISPLAY_REFRESH                             0x12
#define DATA_START_TRANSMISSION_2                   0x13
#define PARTIAL_DATA_START_TRANSMISSION_1           0x14
#define PARTIAL_DATA_START_TRANSMISSION_2           0x15
#define PARTIAL_DISPLAY_REFRESH                     0x16
#define LUT_FOR_VCOM                                0x20
#define LUT_WHITE_TO_WHITE                          0x21
#define LUT_BLACK_TO_WHITE                          0x22
#define LUT_WHITE_TO_BLACK                          0x23
#define LUT_BLACK_TO_BLACK                          0x24
#define PLL_CONTROL                                 0x30
#define TEMPERATURE_SENSOR_COMMAND                  0x40
#define TEMPERATURE_SENSOR_CALIBRATION              0x41
#define TEMPERATURE_SENSOR_WRITE                    0x42
#define TEMPERATURE_SENSOR_READ                     0x43
#define VCOM_AND_DATA_INTERVAL_SETTING              0x50
#define LOW_POWER_DETECTION                         0x51
#define TCON_SETTING                                0x60
#define TCON_RESOLUTION                             0x61
#define SOURCE_AND_GATE_START_SETTING               0x62
#define GET_STATUS                                  0x71
#define AUTO_MEASURE_VCOM                           0x80
#define VCOM_VALUE                                  0x81
#define VCM_DC_SETTING_REGISTER                     0x82
#define PROGRAM_MODE                                0xA0
#define ACTIVE_PROGRAM                              0xA1
#define READ_OTP_DATA                               0xA2
// original
const unsigned char lut_vcom_dc[] = {
    0x00, 0x00,
    0x00, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x00, 0x0E, 0x01, 0x0E, 0x01, 0x10,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char lut_ww[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x40, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x10,
    0x80, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char lut_bw[] = {
    0xA0, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x10,
    0x90, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0xB0, 0x04, 0x10, 0x00, 0x00, 0x05,
    0xB0, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0xC0, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char lut_bb[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x40, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x10,
    0x80, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char lut_wb[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x20, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x10,
    0x10, 0x0A, 0x0A, 0x00, 0x00, 0x08,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};

// fast
const unsigned char fast_lut_vcom_dc[] = {
    0x00, 0x00,
    0x00, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x00, 0x0E, 0x01, 0x0E, 0x01, 0x01,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char fast_lut_ww[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x40, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x01,
    0x80, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char fast_lut_bw[] = {
    0xA0, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x00, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x01,
    0x90, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0xB0, 0x04, 0x10, 0x00, 0x00, 0x05,
    0xB0, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0xC0, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char fast_lut_bb[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x40, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x01,
    0x80, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};
const unsigned char fast_lut_wb[] = {
    0x90, 0x1A, 0x1A, 0x00, 0x00, 0x01,
    0x20, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x84, 0x0E, 0x01, 0x0E, 0x01, 0x01,
    0x10, 0x0A, 0x0A, 0x00, 0x00, 0x01,
    0x00, 0x04, 0x10, 0x00, 0x00, 0x05,
    0x00, 0x03, 0x0E, 0x00, 0x00, 0x0A,
    0x00, 0x23, 0x00, 0x00, 0x00, 0x01
};

void SendCommand(UBYTE command) {
    EpdIf::DigitalWrite(DC_PIN, LOW);
    EpdIf::DigitalWrite(CS_PIN, LOW);
    EpdIf::SpiTransfer(command);
    EpdIf::DigitalWrite(CS_PIN, HIGH);
}

void SendData(UBYTE data) {
    EpdIf::DigitalWrite(DC_PIN, HIGH);
    EpdIf::DigitalWrite(CS_PIN, LOW);
    EpdIf::SpiTransfer(data);
    EpdIf::DigitalWrite(CS_PIN, HIGH);
}


void WaitUntilIdle(void) {
    while(EpdIf::DigitalRead(BUSY_PIN) == 0) {      //0: busy, 1: idle
        EpdIf::DelayMs(100);
    }
}

void Reset(void) { //done
    EpdIf::DigitalWrite(RST_PIN, HIGH);
    EpdIf::DelayMs(200);
    EpdIf::DigitalWrite(RST_PIN, LOW);                //module reset
    EpdIf::DelayMs(200);
    EpdIf::DigitalWrite(RST_PIN, HIGH);
    EpdIf::DelayMs(200);
}

void SetLut(bool fastLut) {
    unsigned int count;

    SendCommand(LUT_FOR_VCOM);                            //vcom
    for(count = 0; count < 44; count++) {
        SendData((fastLut ? fast_lut_vcom_dc : lut_vcom_dc)[count]);
    }

    SendCommand(LUT_WHITE_TO_WHITE);                      //ww --
    for(count = 0; count < 42; count++) {
        SendData((fastLut ? fast_lut_ww : lut_ww)[count]);
    }

    SendCommand(LUT_BLACK_TO_WHITE);                      //bw r
    for(count = 0; count < 42; count++) {
        SendData((fastLut ? fast_lut_bw : lut_bw)[count]);
    }

    SendCommand(LUT_WHITE_TO_BLACK);                      //wb w
    for(count = 0; count < 42; count++) {
        SendData((fastLut ? fast_lut_bb : lut_bb)[count]);
    }

    SendCommand(LUT_BLACK_TO_BLACK);                      //bb b
    for(count = 0; count < 42; count++) {
        SendData((fastLut ? fast_lut_wb : lut_wb)[count]);
    }
}

void width(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Number> num = Number::New(isolate, EPD_WIDTH);
  args.GetReturnValue().Set(num);
}

void height(const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = args.GetIsolate();
  Local<Number> num = Number::New(isolate, EPD_HEIGHT);
  args.GetReturnValue().Set(num);
}



void init_sync(bool fastLut) {
  if (EpdIf::IfInit() != 0) {
    // TODO : throw error
  }	else {
    Reset();

    SendCommand(POWER_ON);
    WaitUntilIdle();

    SendCommand(PANEL_SETTING);
    SendData(0xaf);        //KW-BF   KWR-AF    BWROTP 0f

    SendCommand(PLL_CONTROL);
    SendData(0x3a);       //3A 100HZ   29 150Hz 39 200HZ    31 171HZ

    SendCommand(POWER_SETTING);
    SendData(0x03);                  // VDS_EN, VDG_EN
    SendData(0x00);                  // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
    SendData(0x2b);                  // VDH
    SendData(0x2b);                  // VDL
    SendData(0x09);                  // VDHR

    SendCommand(BOOSTER_SOFT_START);
    SendData(0x07);
    SendData(0x07);
    SendData(0x17);

    // Power optimization
    SendCommand(0xF8);
    SendData(0x60);
    SendData(0xA5);

    // Power optimization
    SendCommand(0xF8);
    SendData(0x89);
    SendData(0xA5);

    // Power optimization
    SendCommand(0xF8);
    SendData(0x90);
    SendData(0x00);

    // Power optimization
    SendCommand(0xF8);
    SendData(0x93);
    SendData(0x2A);

    // Power optimization
    SendCommand(0xF8);
    SendData(0x73);
    SendData(0x41);

    SendCommand(VCM_DC_SETTING_REGISTER);
    SendData(0x12);
    SendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
    SendData(0x87);        // define by OTP

    SetLut(fastLut);

    SendCommand(PARTIAL_DISPLAY_REFRESH);
    SendData(0x00);
	}
}

void init(const FunctionCallbackInfo<Value>& args) {
	Isolate* isolate = args.GetIsolate();
  bool fastLut = false;

  if(args.Length() > 0 ) {
    if (!args[0]->IsObject()) {
      isolate->ThrowException(Exception::TypeError(
      String::NewFromUtf8(isolate, "Error: object expected")));
      return;
    }

    Local<Context> context = isolate->GetCurrentContext();
    Local<Object> obj = args[0]->ToObject(context).ToLocalChecked();
    Local<Array> props = obj->GetOwnPropertyNames(context).ToLocalChecked();

    for(int i = 0, l = props->Length(); i < l; i++) {
      Local<Value> localKey = props->Get(i);
      Local<Value> localVal = obj->Get(context, localKey).ToLocalChecked();
      std::string key = *String::Utf8Value(localKey);
      // std::string val = *String::Utf8Value(localVal);
      // std::cout << key << ":" << val << std::endl;
      if (key.compare("fastLut") == 0) {
        // bool fastLut = localVal.ToBoolean();
        // Local<Boolean> flag = localVal;
        // Local<Boolean> flag = obj->Get(context, localKey);
        // bool fastLut = Boolean::BooleanValue(flag);
        fastLut = localVal->NumberValue();
        // std::cout << "setting" << ":" << fastLut << std::endl;
      }
    }
  }
  Nan::AsyncQueueWorker(new Epd2In7AsyncWorker(
    bind(init_sync, fastLut),
    new Nan::Callback(args[1].As<v8::Function>())
  ));
}


void display(UBYTE * blackImage, UBYTE * redImage)
{
    UWORD Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    if (blackImage != NULL) {
      SendCommand(DATA_START_TRANSMISSION_1);
      for (UWORD j = 0; j < Height; j++) {
          for (UWORD i = 0; i < Width; i++) {
            UBYTE buffer = 0xff;
            for (UWORD k = 0; k < 8; k++) {
              buffer = (buffer << 1) | (blackImage[ 8 * (i + j * Width) + k ] & 0x01);
            }
            SendData(buffer);
          }
      }
      SendData(DATA_STOP);
    }

    if (redImage != NULL) {
      SendCommand(DATA_START_TRANSMISSION_2);
      for (UWORD j = 0; j < Height; j++) {
          for (UWORD i = 0; i < Width; i++) {
            UBYTE buffer = 0xff;
            for (UWORD k = 0; k < 8; k++) {
              buffer = (buffer << 1) | (redImage[ 8 * (i + j * Width) + k ] & 0x01);
            }
            SendData(buffer);
          }
      }
      SendData(DATA_STOP);
    }

    SendCommand(DISPLAY_REFRESH);
    WaitUntilIdle();
}

// TODO : dig in partial refresh
void partialDisplay(UBYTE * Imageblack, UBYTE * Imagered, UWORD x, UWORD y, UWORD w, UWORD h)
{
    UWORD Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    SendCommand(PARTIAL_DATA_START_TRANSMISSION_1);
    // send x
    SendData((UBYTE) (x >> 8));
    SendData((UBYTE) (x & (0xff << 3)));
    //send y
    SendData((UBYTE) (y >> 8));
    SendData((UBYTE) y);
    //send w
    SendData((UBYTE) (w >> 8));
    SendData((UBYTE) (w & (0xff << 3)));
    //send h
    SendData((UBYTE) (h >> 8));
    SendData((UBYTE) h);

    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
          UBYTE buffer = 0xff;
          for (UWORD k = 0; k < 8; k++) {
            buffer = (buffer << 1) | (Imageblack[ 8 * (i + j * Width) + k ] & 0x01);
          }
          SendData(buffer);
        }
    }
    SendData(DATA_STOP);

    // if (Imagered != NULL) {
    //   SendCommand(DATA_START_TRANSMISSION_2);
    //   for (UWORD j = 0; j < Height; j++) {
    //       for (UWORD i = 0; i < Width; i++) {
    //           SendData(Imagered[i + j * Width]);
    //       }
    //   }
    //   SendData(DATA_STOP);
    // }

    SendCommand(PARTIAL_DISPLAY_REFRESH);
    // send x
    SendData((UBYTE) (x >> 8));
    SendData((UBYTE) x);
    //send y
    SendData((UBYTE) (y >> 8));
    SendData((UBYTE) y);
    //send w
    SendData((UBYTE) (w >> 8));
    SendData((UBYTE) w);
    //send h
    SendData((UBYTE) (h >> 8));
    SendData((UBYTE) h);

    WaitUntilIdle();
}


void displayFrame(const FunctionCallbackInfo<Value>& args) {
  UBYTE* blackImageData = NULL;
  UBYTE* redImageData = NULL;

  if ( ! args[0]->IsNull()) {
  	v8::Local<v8::Uint8Array> blackView = args[0].As<v8::Uint8Array>();
  	void *blackData = blackView->Buffer()->GetContents().Data();
  	blackImageData = static_cast<UBYTE*>(blackData);
  }

  if ( ! args[1]->IsNull()) {
    v8::Local<v8::Uint8Array> redView = args[1].As<v8::Uint8Array>();
  	void *redData = redView->Buffer()->GetContents().Data();
  	redImageData = static_cast<UBYTE*>(redData);
  }

  Nan::AsyncQueueWorker(new Epd2In7AsyncWorker(
    bind(display, blackImageData, redImageData),
    new Nan::Callback(args[2].As<v8::Function>())
  ));
}



void clear_sync(void) {
    UWORD Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;

    SendCommand(DATA_START_TRANSMISSION_1);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            SendData(0X00);
        }
    }
    SendData(DATA_STOP);

    SendCommand(DATA_START_TRANSMISSION_2);
    for (UWORD j = 0; j < Height; j++) {
        for (UWORD i = 0; i < Width; i++) {
            SendData(0X00);
        }
    }
    SendData(DATA_STOP);

    SendCommand(DISPLAY_REFRESH);
    WaitUntilIdle();
}

void clear(const FunctionCallbackInfo<Value>& args) {
  Nan::AsyncQueueWorker(new Epd2In7AsyncWorker(
    bind(clear_sync),
    new Nan::Callback(args[0].As<v8::Function>())
  ));
}

void sleep_sync(void) {
  SendCommand(0X50);
  SendData(0xf7);
  SendCommand(0X02);  	//power off
  SendCommand(0X07);  	//deep sleep
  SendData(0xA5);
}

void sleep(const FunctionCallbackInfo<Value>& args) {
  Nan::AsyncQueueWorker(new Epd2In7AsyncWorker(
    bind(sleep_sync),
    new Nan::Callback(args[0].As<v8::Function>())
  ));
}

void InitAll(Local<Object> exports) {
  NODE_SET_METHOD(exports, "init", init);
  NODE_SET_METHOD(exports, "clear", clear);
  NODE_SET_METHOD(exports, "sleep", sleep);
  NODE_SET_METHOD(exports, "width", width);
  NODE_SET_METHOD(exports, "height", height);
  NODE_SET_METHOD(exports, "displayFrame", displayFrame);
}

NODE_MODULE(epd2in7b, InitAll)
