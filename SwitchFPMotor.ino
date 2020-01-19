/**
 * PROJECT SWITCH POWER SEPEDA MOTOR DENGAN FINGERPRINT SENSOR
 * Copyright (c) 2019. Dwi Candra Permana.
 * 
 * Credits
 * ----------
 * - Penulis Kode         : Dwi Candra Permana
 * 
 * Deskripsi:
 * ----------
 * Ketika sidik jari sudah dibaca dan berhasil, akan dilakukan suatu aksi menurut beberapa keadaan:
 * - Jika dalam keadaan motor mati, kontak motor akan dihidupkan
 * - Jika kontak sudah dihidupkan, mesin motor akan dijalankan
 * - Jika mesin dalam keadaan nyala, motor akan dimatikan
 * Namun, apabila melakukan kesalahan sebanyak yang ditentukan oleh variabel FAILED_COUNT_MAX, 
 * maka motor akan dimatikan secara paksa dan akses akan diblokir selama waktu yang ditentukan
 * di variabel SUSPEND_TIME.
 * 
 * Fitur:
 * ----------
 * - Menghidupkan mesin motor dan mematikan motor dengan sidik jari
 * - Penghapusan seluruh data sidik jari
 * - Pendaftaran (Enroll) sidik jari hingga 120 sidik jari
 * - Keluaran suara indikator untuk melakukan aksi tertentu dengan file MP3
 * 
 * Alat dan Bahan:
 * ----------
 * - Arduino/Genuino UNO Rev. 3 (ATMega 328P)
 * - Adafruit Fingerprint FPM10A
 * - Modul Relay 2-Channel
 * - DFPlayer Mini
 * - Speaker Output
 * - Beberapa Kabel Jumper
 * 
 */

#include <SoftwareSerial.h>         // memasukan library Software Serial
#include <DFPlayer_Mini_Mp3.h>      // memasukan library DFPlayermini
#include <Adafruit_Fingerprint.h>
#define FINGER_TX_IN 2              // pin #2 is IN/TX from sensor (GREEN wire)
#define FINGER_RX_OUT 3             // pin #3 is OUT/RX from arduino  (WHITE wire)
#define ACT_BUTTON 4
#define INDICATOR 5
#define DFPLAYER_TX 6
#define DFPLAYER_RX 7
#define CONTACT 8
#define STARTER 9

// Fingerprint Sensor Ports
SoftwareSerial fingerSerial(FINGER_TX_IN, FINGER_RX_OUT);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerSerial);
uint8_t id;

// DFPlayer Serial Ports
SoftwareSerial MP3Serial(DFPLAYER_TX, DFPLAYER_RX);

// Button
unsigned long keyPrevMillis               = 0;
const unsigned long keySampleIntervalMs   = 25;
byte longKeyPressCountMax                 = 80;    // 80 * 25 = 2000 ms
byte longKeyPressCount                    = 0;
byte prevKeyState                         = HIGH;         // button is active low
const byte keyPin                         = ACT_BUTTON;            // button is connected to pin 4 and GND

int STATE                       = -1; //status keadaan, -1 = off (default state), 0 = mesin hidup, 1 = motor hidup, 2 = mesin mati
const int ENGINE_HOLD_DELAY     = 3000; // durasi saat starter motor dihubungkan
int FAILED_COUNT                = 0;
const int FAILED_COUNT_MAX      = 5; // maksimum jumlah kesalahan
const int SUSPEND_TIME          = 30000; // 5 menit

// Nomor File MP3
const int PLAY_WELCOME                    = 1;
const int PLAY_START_ENGINE               = 2;
const int PLAY_ACCESS_DENIED              = 3;
const int PLAY_ACCESS_SUSPEND             = 4;
const int PLAY_ENROLL_FINGER              = 5;
const int PLAY_ENROLL_SECOND              = 6;
const int PLAY_ENROLL_SAME_FINGER_AGAIN   = 7;
const int PLAY_ENROLL_SUCCESS             = 8;
const int PLAY_ENROLL_MISMATCH            = 9;
const int PLAY_ENROLL_FINGER_FULL         = 10;
const int PLAY_CLEAN_FINGER               = 11;
const int PLAY_CLEAN_COMPLETE             = 12;

// called when button is kept pressed for less than 2 seconds
void nothingKeyPress() {
    if (STATE == 1) {
        digitalWrite(CONTACT, LOW);
        STATE = -1;
    } else {
        Serial.println("Nothing to do.");
    }
}

void deleteKeyPress() {
    // delete db finger
    finger.emptyDatabase();
}

// called when button is kept pressed for more than 2 seconds
void enrollKeyPress() {
    // enroll finger
    Serial.println("Ready to enroll a fingerprint!");
    //Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
    finger.getTemplateCount();
    if (finger.templateCount <= 121) {
      id = finger.templateCount + 1;
      //id = readnumber();
      if (id == 0) {// ID #0 not allowed, try again!
         return;
      }
      Serial.print("Enrolling ID #");
      Serial.println(id);
      
      while (!  getFingerprintEnroll() );
    } else {
      mp3_play(PLAY_ENROLL_FINGER_FULL);
      delay(3000);
    }
}


// called when key goes from not pressed to pressed
void keyPress() {
    Serial.println("key press");
    longKeyPressCount = 0;
}


// called when key goes from pressed to not pressed
void keyRelease() {
    Serial.println("key release");
    if (longKeyPressCount >= 320) {
        nothingKeyPress();
    } else if ((longKeyPressCount >= 200) && (longKeyPressCount < 320)) {
        deleteKeyPress(); //5s - 8s
    } else if ((longKeyPressCount >= 80) && (longKeyPressCount < 200)) {
        enrollKeyPress(); //2s - 5s
    } else {
        //nothingKeyPress();
    }
}

uint8_t readnumber(void) {
  uint8_t num = 0;
  
  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  mp3_play(PLAY_ENROLL_FINGER);
  delay(500);
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
      p = finger.getImage();
      switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println(".");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
      }
  }
  
  // OK success!
  
  p = finger.image2Tz(1);
  switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
      case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
      case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
      default:
        Serial.println("Unknown error");
        return p;
  }
  
  Serial.println("Remove finger");
  mp3_play(PLAY_ENROLL_SECOND); 
  delay(500);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  mp3_play(PLAY_ENROLL_SAME_FINGER_AGAIN);
  delay(500);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }
  
  // OK success!
  
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    mp3_play(PLAY_ENROLL_MISMATCH);
    delay(500);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    mp3_play(PLAY_ENROLL_SUCCESS);
    delay(500);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    // gak terdaftar
    Serial.println("Did not find a match");
    if (FAILED_COUNT < FAILED_COUNT_MAX) {
        FAILED_COUNT++;
    } else {
        // akses diblokir
        digitalWrite(CONTACT, LOW);
        STATE = -1;
        mp3_play(PLAY_ENROLL_FINGER);
        delay(SUSPEND_TIME);
        FAILED_COUNT = 0;
    }
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 
  FAILED_COUNT = 0; // reset jumlah kesalahan

  return finger.fingerID;
}

void setup() {
    Serial.begin(9600);

    // button
    pinMode(keyPin, INPUT_PULLUP);

    // dfplay
    mp3_set_serial(MP3Serial); 
    delay(5); 
    mp3_set_volume(15);

    //finger
    while (!Serial);  // For Yun/Leo/Micro/Zero/...
    delay(100);
    Serial.println("\n\nAdafruit Fingerprint sensor enrollment");
  
    // set the data rate for the sensor serial port
    finger.begin(57600);
    
    if (finger.verifyPassword()) {
      Serial.println("Found fingerprint sensor!");
    } else {
      Serial.println("Did not find fingerprint sensor :(");
      while (1) { delay(1); }
    }

    finger.getTemplateCount();
    Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
    Serial.println("Waiting for valid finger...");
}

void loop() {
    // key management section
    if (millis() - keyPrevMillis >= keySampleIntervalMs) {
        keyPrevMillis = millis();
       
        byte currKeyState = digitalRead(keyPin);
       
        if ((prevKeyState == HIGH) && (currKeyState == LOW)) {
            keyPress();
        } else if ((prevKeyState == LOW) && (currKeyState == HIGH)) {
            keyRelease();
        } else if (currKeyState == LOW) {
            longKeyPressCount++;

            // kedipan lampu
            switch (longKeyPressCount) {
              case 80:
                break;
              case 200:
                break;
              case 320:
                break;
              default:
                break;
            }
        }
       
        prevKeyState = currKeyState;
    }

    // other code goes here
    // get finger id
    getFingerprintID();
    if (getFingerprintID() != -1) {
      STATE++;
      switch (STATE) {
        case 0:
          digitalWrite(CONTACT, HIGH);
          mp3_play(PLAY_WELCOME);
          delay(500);
          break;
        case 1:
          mp3_play(PLAY_START_ENGINE);
          delay(500);
          digitalWrite(CONTACT, HIGH);
          digitalWrite(STARTER, HIGH);
          delay(ENGINE_HOLD_DELAY); // delay untuk starter motor
          digitalWrite(STARTER, LOW);
          break; 
        case 2:
          digitalWrite(CONTACT, LOW);
          STATE = -1;
          break;
        default:
          break;
      }
    }

    delay(50);            //don't ned to run this at full speed.
}
