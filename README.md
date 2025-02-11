Project Smart Air Purifier: An air cleaner that tracks information and evaluates the levels of PM dust, temperature, humidity, gas in your home. This is a student project created by electrical technology students at Roi Et Technical College in Thailand.




Email-Receveiver [ code สำรหับส่ง Email โดยการอ่านข้อมูลจาก Thingspeac ]

Master [ Board DOIT ESP32 DEVKITV1  master ตัวกลางการสื่อสารกับ board อื่นๆ ]

Water_spray_machine [ Board Generic ESP8266 Module ]

Smart_air_purifier [ Board DOIT ESP32 DEVKITV1  SMART_AIR_V3.1 
 - ปรับการเก็บข้อมูลเพื่อคำนวนค่า AQI จาก 24h. เป็น 5minut เพื่อลดภาระของระบบ
 - ปรับ code ให้ใช้งานร่วมกับ " project Development of smart air purifiers "
 - เพิ่มระบบ ป้องกันหากอุณหภูมิ MCU ESP32 สูงเกินไปพัดลมจะทำงานเพื่อลดอุณหภูมิในระบบ  โดยอ่านค่าอุณหภูมิจากเซ็นเซอร์ภายใน ESP32
       | ESP32 มี Maximum Operating Temperature ที่ประมาณ 125°C ตามสเปกชิป (ขึ้นอยู่กับรุ่นย่อย) แต่ในความเป็นจริง การทำงานที่อุณหภูมิสูงเกิน 70-80°C
         อาจทำให้อายุการใช้งานของชิปลดลง หรือเกิดปัญหาเสถียรภาพในระยะยาวได้ เช่น รีเซ็ตตัวเองหรือการประมวลผลผิดพลาด
         ค่า 70°C เป็นการตั้งระดับความปลอดภัยที่ต่ำกว่า Maximum Temperature เพื่อให้มั่นใจว่าระบบจะทำงานได้อย่างเสถียรในระยะยาว 
         **ใช้การกรองสัญญาณรบกวน (Filtering) แบบ Low-Pass Filter หรือ Kalman Filter เพื่อปรับปรุงค่าที่อ่านให้เสถียรมากขึ้น  ใช้ 65°C ในการให้ระบบป้องกันเริ่มทำงาน
         **ใช้ sensor DH22 ประกอบการทำงานผ่านตัวแปร "temperature_sensor"

  Sketch ใช้พื้นที่จัดเก็บโปรแกรม 920,513 ไบต์ (70%) พื้นที่สูงสุดคือ 1310,720 ไบต์
  ]
