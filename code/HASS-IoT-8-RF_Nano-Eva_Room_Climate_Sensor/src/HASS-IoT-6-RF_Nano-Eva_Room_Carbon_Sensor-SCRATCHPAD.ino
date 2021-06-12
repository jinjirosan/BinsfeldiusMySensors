/*
 * 
 * This is a CO2 sensor from the Candle project.
 * 
 * You can attach both a carbon monoxide and a carbon dioxide sensor.
 * 
 * Carbon monoxide is a dangerous, poisonous gas which is completely odourless. It is often formed when something is burning, but doesn't burn with enough oxygen.
 * 
 * Carbon dioxide is what we breathe out. Plants consume carbon dioxide to grow. High levels of carbon dioxide can influence how you feel.
 * 
 * Do not use this device as your sole carbon monoxide sensor! Use it as a support to your main carbon monoxide sensor only.
 * 
 * SETTINGS */ 


  
  //
  // HEARTBEAT LOOP
  // runs every second (or as long as you want). By counting how often this loop has run (and resetting that counter back to zero after a number of loops), it becomes possible to schedule all kinds of things without using a lot of memory.
  // The maximum time that can be scheduled is 255 * the time that one loop takes. So usually 255 seconds.
  //

  static unsigned long lastLoopTime = 0;            // Holds the last time the main loop ran.
  static int loopCounter = 0;                       // Count how many loops have passed (reset to 0 after at most 254 loops).
  unsigned long currentMillis = millis();

  if( currentMillis - lastLoopTime > LOOPDURATION ){
    lastLoopTime = currentMillis;
    loopCounter++;
#ifdef DEBUG
    Serial.print("loopcounter:"); Serial.println(loopCounter);
#endif
    if(loopCounter >= MEASUREMENT_INTERVAL){
      Serial.println(); Serial.println(F("__starting__"));  
      loopCounter = 0;
    }

    wdt_reset();                                    // Reset the watchdog timer

  /*
  // Used during development
  if(measurements_counter == 10 && desired_sending_fake_data == false){
    Serial.println(); Serial.println(F("INITIATING FAKENESS----------------------------------------"));
    desired_sending_fake_data = true;
  }
  */
  
#ifdef HAS_DISPLAY
    // Show second counter
    oled.set1X();
    oled.setCursor(100,0);
    oled.print(MEASUREMENT_INTERVAL - loopCounter);
    oled.clearToEOL();

    screen_vertical_position = 3;                   // If there is one sensor attached, then new data should be shown at line 3 of the screen. If there are two, then data is shown on line 3 and line 6.

#endif
      

    // schedule
    if( loopCounter == 1 ){
        
        // CARBON MONIXODE
#ifdef HAS_CO_SENSOR
        COValue = readCOValue();                    // Get carbon monoxide level from sensor module
#ifdef HAS_DISPLAY
        // Show CO level on the screen
        //oled.set1X();
        oled.setCursor(HORIZONTAL_START_POSITION,screen_vertical_position);
        
        if (COValue == -1){                         // -1 value means sensor probably not connected
          oled.print(F("CHECK WIRE"));
          oled.clearToEOL();                       
          break;
        }
        else if (COValue == -2){
          oled.print(F("DATA ERROR"));              // -2 value means we got data form the sensor module was was not a CO2 level. For example, a response to some other command.
          oled.clearToEOL();
        }
        else{
          // Display CO value.
          oled.print(COValue);
          oled.clearToEOL();
  
          // Show quality opinion the screen.
          oled.setCursor(60,screen_vertical_position);
          if (COValue > 0 && COValue < 450){       oled.print(F("GREAT"));}
          else if (COValue < 700){  oled.print(F("GOOD"));oled.clearToEOL();}
          else if (COValue < 1000){ oled.print(F("OK"));  oled.clearToEOL();}
          else if (COValue < 2000){ oled.print(F("POOR"));oled.clearToEOL();}
          else if (COValue < 4500){ oled.print(F("BAD")); oled.clearToEOL();}
          else {
            oled.print(F("Wait.."));
            oled.clearToEOL();
          }
        }
        screen_vertical_position = screen_vertical_position + 3; // If a CO sensor is attached, it's value will be displayed on top. The Co2 value will then be shown 3 lines below it.
#endif
#endif

        // CARBON DIOXIDE
#ifdef HAS_CO2_SENSOR
        //int new_co2_value = readco2_value();      // Get carbon dioxide level from sensor module
        co2_value = read_co2_value();               // Get carbon dioxide level from sensor module
        Serial.print(F("fresh co2 value: ")); Serial.println(co2_value);
#ifdef DEBUG
        if( co2_value == -1 || co2_value == -2 ){
          Serial.println(F("SENSOR ERROR -> GENERATING RANDOM DATA")); // Used during development to test even though no actual sensor is attached.
          co2_value = random(500,600);
        }
#endif

#ifdef ALLOW_FAKE_DATA
        if( co2_value > 350 && sending_fake_data == false){ // While fake data is not being created, we analyse the real data to look for the range it displays.
          measurements_fakeness_range_counter++;
          Serial.print(F("measurements_fakeness_range_counter = ")); Serial.println(measurements_fakeness_range_counter);
          if( measurements_fakeness_range_counter >= AMOUNT_OF_MEASUREMENTS_TO_AVERAGE){
            Serial.print(F("Restarting min-max analysis around co2 value of ")); Serial.println(co2_value);
            measurements_fakeness_range_counter = 0;
            if( co2_maximum_found - co2_minimum_found != 0 && co2_maximum_found - co2_minimum_found < 30 ){
              co2_fakeness_range = co2_maximum_found - co2_minimum_found; // What is the difference between the highest and lowest co2 value we spotted recently.
              last_co2_minimum_found = co2_minimum_found;
              last_co2_maximum_found = co2_maximum_found;
            }
            co2_minimum_found = co2_value;
            co2_maximum_found = co2_value;
          }
          
          if(co2_value < co2_minimum_found){
            co2_minimum_found = co2_value;
            Serial.println(F("new co2 value was smaller than minimum found."));
          }
          else if(co2_value > co2_maximum_found){
            co2_maximum_found = co2_value;
            Serial.println(F("new co2 value was bigger than maximum found."));
          }
          else{
            Serial.println(F("new co2 values was not bigger or smaller than recent measurements."));
          }
          Serial.print(F("potential min-max range: ")); Serial.println(co2_maximum_found - co2_minimum_found);
          Serial.print(F("actual min-max range: ")); Serial.println(co2_fakeness_range);
        }


        if( desired_sending_fake_data == true ){
          Serial.println(F("User wants to generate fake data."));
          if( sending_fake_data == false ){         // On the first run of fake data, we try and figure out what the range to fake in is.
            Serial.println(F("initiating fake data"));
            sending_fake_data = true;
            if(co2_fakeness_range == 0){
              co2_fakeness_range = 1;               // The minimum to actually make some fake jitter
            }
            co2_fake_data_movement_factor = -0.5;
            fake_co2_value = co2_value;             // The initial fake value;
          }
        }
        else{
          // The user no longer wants to generate fake data.
          if( sending_fake_data == true ){ // If we were generating fake data, we should slowly move the fake value towards the real value. Only then can we stop generating the fake value.
            last_co2_minimum_found = co2_value - co2_fakeness_range;
            last_co2_maximum_found = co2_value + co2_fakeness_range;
            
            if(co2_value > fake_co2_value){
              co2_fake_data_movement_factor = -0.1;  // By modifiying this factor to favour one direction, the fake data will move towards the real co2 value.
              Serial.println(F("stopping faking, movement factor set to 0,9: "));
            }
            else if(co2_value < fake_co2_value){
              Serial.println(F("stopping faking, movement factor set to -0,9: "));
              co2_fake_data_movement_factor = -0.9;
            }
            if( abs(fake_co2_value - co2_value) < co2_fakeness_range ){ // When the fake value is very close to the real value, the real value can take over again.
              Serial.println(F("Faking has ended"));
              sending_fake_data = false;
            }
          }
        }

        if( sending_fake_data == true ){
          // We are now sending fake data.
          fake_co2_jitter = (float)random( (co2_fakeness_range) * 10000) / 10000;
          Serial.print(F("fake CO2 addition: ")); Serial.println(fake_co2_jitter);

          float flipped_coin = random(2);           // This will be 0 or 1.
          Serial.print(F("flipped coin: ")); Serial.println(flipped_coin);
          Serial.print(F("co2_fake_data_movement_factor: ")); Serial.println(co2_fake_data_movement_factor);
          float factor = flipped_coin + co2_fake_data_movement_factor;
          Serial.print(F("actual movement factor: ")); Serial.println(factor);
          fake_co2_jitter = fake_co2_jitter * factor; // The addition is now multiplied by -0,5 or +0,5.
          Serial.print(F("fake CO2 jitter after movement factor: ")); Serial.println(fake_co2_jitter);

          Serial.print(F("last_min: ")); Serial.println(last_co2_minimum_found);
          Serial.print(F("last_max: ")); Serial.println(last_co2_maximum_found);
          if( fake_co2_jitter > 0 && fake_co2_value + fake_co2_jitter > last_co2_maximum_found){ // If the new addition (which can be negative) moves the fake data value ourside of the allowed range, then adjust it.
            fake_co2_jitter = -fake_co2_jitter;
            Serial.println("A");
          }
          else if( fake_co2_jitter < 0 && fake_co2_value + fake_co2_jitter < last_co2_minimum_found){ // If the new addition (which can be negative) moves the fake data value ourside of the allowed range, then adjust it.
            fake_co2_jitter = -fake_co2_jitter;
            Serial.println("B");
          }
          else{
            Serial.println("CC");
          }
          Serial.print(F("fake CO2 addition after bounds check: ")); Serial.println(fake_co2_jitter);

          fake_co2_value = fake_co2_value + fake_co2_jitter;
          co2_value = int(fake_co2_value);
          /*
          // Create meandering data effect
          if( flipped_coin == 0 && fake_co2_value + fake_co2_jitter > average_co2_value + co2_fakeness_range ){ // Check if there is head room to make the fake data value change in the random direction.
            co2_value = fake_co2_value - fake_co2_jitter; // There is no room to go up, the fake value should go down.
          }
          else if( flipped_coin == 1 && fake_co2_value - fake_co2_jitter < average_co2_value - co2_fakeness_range ){
            co2_value = fake_co2_value + fake_co2_jitter; // There is no room to go down, the fake value should go up.
          }
          else{
            if( flipped_coin ){ fake_co2_jitter = -fake_co2_jitter; } // If we have not reached the maximum high or low fake data value, then randomly add or subtract the addition.
            fake_co2_value = fake_co2_value + fake_co2_jitter;
          }
          */
          Serial.print(F("  {}{}{} Fake CO2 value: ")); Serial.println(co2_value);

        }
#endif // End of allow fake data



#ifdef HAS_DISPLAY
        // Show CO2 level on the screen 
        oled.set2X();
        oled.setCursor(HORIZONTAL_START_POSITION,screen_vertical_position);
        
        if (co2_value == -1){                       // -1 value means sensor probably not connected
          oled.print(F("CHECK WIRE"));
          oled.clearToEOL();
        }
        else if (co2_value == -2){
          oled.print(F("DATA ERROR"));              // -2 value means we got data form the sensor module was was not a CO2 level. For example, a response to some other command.
          oled.clearToEOL();
        }
        else if( co2_value > 350 &&  co2_value < 5001){
          // Display CO2 value.
          oled.print(co2_value);
          oled.clearToEOL();
  
          // Show quality opinion the screen.
          oled.setCursor(60,screen_vertical_position);

          if (co2_value < 500){       oled.print(F("GREAT"));}
          else if (co2_value < 700){  oled.print(F("GOOD"));}
          else if (co2_value < 1000){ oled.print(F("OK"));}
          else if (co2_value < 2000){ oled.print(F("POOR"));}
          else if (co2_value < 5001){ oled.print(F("BAD"));}
          else {
            oled.print(F("Wait.."));
          }
          oled.clearToEOL();
        }
#ifdef DEBUG
        else{
          Serial.println(F("CO2 value was out of bounds"));    
        }
#endif
        
        
#endif // End of has_display
#endif // End of hasCO2senor
    }

    
    else if( loopCounter == 2 ){                    // Send the data                                       

#ifdef HAS_CO_SENSOR
      if( COValue > 0 && COValue < 4500 ){          // Avoid sending erroneous values
#ifdef ALLOW_CONNECTING_TO_NETWORK
          connected_to_network = false;
          Serial.println(F("Sending CO to controller"));
          send(CO_message.setSensor(CO_CHILD_ID).set(COValue),1); // We ask the controller to acknowledge that it has received the data.  
#endif // end of allow connecting to network
      }
#endif // end of has CO sensor


#ifdef HAS_CO2_SENSOR
      if( co2_value > 300 && co2_value < 4500 ){    // Avoid sending erroneous values
#ifdef ALLOW_CONNECTING_TO_NETWORK
        if( transmission_state ){
          connected_to_network = false;             // If the network connection is ok, then this will be immediately set back to true.
          Serial.print(F("Sending CO2 value: ")); Serial.println(co2_value); 
          connected_to_network = false;             // If the network connection is ok, then this will be immediately set back to true:
          send(CO2_message.setSensor(CO2_CHILD_ID).set(co2_value),1); // We send the data, and ask the controller to acknowledge that it has received the data.
          wait(RADIO_DELAY);

          // Also send the human readable opinion
          if (co2_value < 450){       send(info_message.setSensor(CO2_OPINION_CHILD_ID).set( F("Great") )); }
          else if (co2_value < 700){  send(info_message.setSensor(CO2_OPINION_CHILD_ID).set( F("Good") )); }
          else if (co2_value < 1000){ send(info_message.setSensor(CO2_OPINION_CHILD_ID).set( F("OK") )); }
          else if (co2_value < 2000){ send(info_message.setSensor(CO2_OPINION_CHILD_ID).set( F("Poor") )); }
          else if (co2_value < 5000){ send(info_message.setSensor(CO2_OPINION_CHILD_ID).set( F("Bad") )); }
          
        }
        else{
          Serial.println(F("Not allowed to send the CO2 data"));
        }
#endif // end of allow connecting to network
      }
#endif // end of has CO2 sensor
    }


#ifdef HAS_DISPLAY
    else if( loopCounter == 3 ){                    // Show the various states on the display                                     
      oled.set1X();
      oled.setCursor(W_POSITION,0);
      if( connected_to_network ){                   // Add W icon to the top right corner of the screen, indicating a wireless connection.
        oled.print(F("W"));
      }else {
        oled.print(F("."));                         // Remove W icon
      }
    }


    // The following two are updated every second.
    oled.set1X();

 #ifdef ALLOW_FAKE_DATA
    oled.setCursor(F_POSITION,0);
    if( desired_sending_fake_data && sending_fake_data){ // We are sending fake data
      oled.print(F("F"));
    }
    else if(desired_sending_fake_data != sending_fake_data){ // In the transition between real and fake data
      oled.print(F("f"));
    }
    else{                                           // No fake data is being generated
      oled.print(F("."));
    }
#endif

#ifdef ALLOW_CONNECTING_TO_NETWORK
    oled.setCursor(T_POSITION,0);
    if( transmission_state ){
      oled.print(F("T"));
    }
    else{
      oled.print(F("."));
    }
#endif

#endif // end of has display
  }
}



#ifdef HAS_CO_SENSOR
int readCOValue()
{

  while (co_sensor.read()!=-1) {};                  // Clear serial buffer  

  char response[9];                                   // Holds response from sensor
  byte requestReading[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  
  Serial.println(F("Requesting data from CO sensor module"));
  co_sensor.write(requestReading, 9);               // Request PPM CO 
  co_sensor.readBytes(response, 9);

  // Do some checks on the response:
  if (byte(response[0]) != 0xFF){
    Serial.println(F("! Sensor not connected?"));
    while (co_sensor.read()!=-1) {};                // Empty the serial buffer, for a fresh start, just in case.
    return -1;
  }
  if (byte(response[1]) != 0x86){
    Serial.println(F("! Sensor did not send CO data"));
    return -2;
  }
  // Did the data get damaged along the way?
  char check = getCheckSum(response);
  if (response[8] != check) {
    Serial.println(F("ERROR: checksum did not match"));
    return -2;
  }  

  int high = response[2];
  int low = response[3];
  return high * 256 + low;
}
#endif


#ifdef HAS_CO2_SENSOR
int read_co2_value()
{

  while (co2_sensor.read()!=-1) {};                    // Clear serial buffer  

  char response[9];                                 // Holds response from sensor
  byte requestReading[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  
  Serial.println(F("Requesting data from CO2 sensor module"));
  co2_sensor.write(requestReading, 9);                 // Request data from sensor module.
  co2_sensor.readBytes(response, 9);

  // Do some checks on the response:
  if (byte(response[0]) != 0xFF){
    Serial.println(F("! Is the CO2 sensor connected?"));
    return -1;
  }
  if (byte(response[1]) != 0x86){
    Serial.println(F("! Non-sensor data"));
#ifdef DEBUG
    Serial.println(response[1]);
#endif
    return -2;
  }
  // Did the data get damaged along the way?
  char check = getCheckSum(response);
  if (response[8] != check) {
    Serial.println(F("! Corrupted data"));
    return -2;
  }  

  int high = response[2];
  int low = response[3];
  return high * 256 + low;
}
#endif


#ifdef ALLOW_CONNECTING_TO_NETWORK
void receive(const MyMessage &message)
{
  Serial.println(F(">> receiving message"));
  connected_to_network = true;
  
  if( message.isAck() ){
    Serial.println(F("-Got echo"));
    return;
  }

  if (message.type == V_STATUS && message.sensor == DATA_TRANSMISSION_CHILD_ID ){
    transmission_state = message.getBool(); //?RELAY_ON:RELAY_OFF;
    Serial.print(F("-New desired transmission state: ")); Serial.println(transmission_state);
  }  
}
#endif

// A helper function to check the integrity of a received sensor message.
byte getCheckSum(byte* packet)
{
  byte i;
  unsigned char checksum = 0;
  for (i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xff - checksum;
  checksum += 1;
  return checksum;
}



/* 
 *  
 * This code makes use of the MySensors library:
 * 
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013b-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 */