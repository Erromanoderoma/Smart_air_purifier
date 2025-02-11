from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
import time
import requests
import smtplib  # Make sure to import smtplib
import json
from datetime import datetime

# Get current date and time
current_datetime = datetime.now().strftime("%d/%m/%Y at %I:%M%p")
print (current_datetime)
# ThingSpeak data
channel_id = "1976729"
read_api_key = "JCH61V9N1JD3ZT36"

# JSON
with open("config.json", "r") as config_file:
    config = json.load(config_file)
    
# Email data
smtp_server = "smtp.gmail.com"
port = 587


# Function to fetch gas data from ThingSpeak
def get_gas_data_from_thingspeak():
    url = f"https://api.thingspeak.com/channels/{channel_id}/feeds.json?api_key={read_api_key}&results=1"
    response = requests.get(url)
    data = response.json()
    feed = data["feeds"][0]
    
    # Extract only the gas field
    gas = feed["field7"]  # Gas data
    return gas

# Function for sending email
def send_email(subject, body):
    try:
        sender_email = config["email"]
        password = config["password"]
        receiver_email = config["recipient"]
        
        # Set MIME
        msg = MIMEMultipart()
        msg["From"] = sender_email
        msg["To"] = receiver_email
        msg["Subject"] = subject

        # Enter email content
        msg.attach(MIMEText(body, "html"))  # Send HTML content in email

        # Connect SMTP and send email
        server = smtplib.SMTP(smtp_server, port)
        server.starttls()  # Encrypt connection
        server.login(sender_email, password)
        server.sendmail(sender_email, receiver_email, msg.as_string())
        server.quit()

        print("Email sent successfully")
    except Exception as e:
        print(f"Error sending email: {e}")

# Start checking and sending notification based on gas level
while True:
    # Example of handling Gas Level
    gas_level_raw = get_gas_data_from_thingspeak()  # Replace with the variable you actually use

    try:
        if gas_level_raw is None:
            print("No Gas Level data found, default to 0")
            gas_level = 0.0  # Default (can be adjusted as desired)
        else:
            gas_level = float(gas_level_raw)  # Convert Gas Level data to float
    except ValueError as e:
        print(f"Invalid Gas Level data: {e}")
        gas_level = 0.0  # Set to default value in case of error

    # Example of using the obtained values
    print(f"Compiled Gas Level: {gas_level}")

    # Check if gas level is greater than or equal to 1
    if gas_level >= 1:
        # HTML content for the email body
            html_content = """
<html dir="ltr" xmlns="http://www.w3.org/1999/xhtml" xmlns:o="urn:schemas-microsoft-com:office:office">
  <head>
    <meta charset="UTF-8">
    <meta content="width=device-width, initial-scale=1" name="viewport">
    <meta name="x-apple-disable-message-reformatting">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta content="telephone=no" name="format-detection">
    <title></title>
    <!--[if (mso 16)]>
    <style type="text/css">
    a {text-decoration: none;}
    </style>
    <![endif]-->
    <!--[if gte mso 9]><style>sup { font-size: 100% !important; }</style><![endif]-->
    <!--[if gte mso 9]>
<noscript>
         <xml>
           <o:OfficeDocumentSettings>
           <o:AllowPNG></o:AllowPNG>
           <o:PixelsPerInch>96</o:PixelsPerInch>
           </o:OfficeDocumentSettings>
         </xml>
      </noscript>
<![endif]-->
  </head>
  <body class="body">
    <div dir="ltr" class="es-wrapper-color">
      <!--[if gte mso 9]>
			<v:background xmlns:v="urn:schemas-microsoft-com:vml" fill="t">
				<v:fill type="tile" color="#f6f6f6"></v:fill>
			</v:background>
		<![endif]-->
      <table width="100%" cellspacing="0" cellpadding="0" class="es-wrapper">
        <tbody>
          <tr>
            <td valign="top" class="esd-email-paddings">
              <table cellspacing="0" cellpadding="0" align="center" class="esd-header-popover es-header">
                <tbody>
                  <tr>
                    <td align="center" class="esd-stripe">
                      <table width="600" cellspacing="0" cellpadding="0" bgcolor="#ffffff" align="center" class="es-header-body">
                        <tbody>
                          <tr>
                            <td align="left" class="es-p20t es-p20r es-p20l esd-structure">
                              <!--[if mso]><table width="560" cellpadding="0"
                            cellspacing="0"><tr><td width="180" valign="top"><![endif]-->
                              <table cellspacing="0" cellpadding="0" align="left" class="es-left">
                                <tbody>
                                  <tr>
                                    <td width="180" valign="top" align="center" class="es-m-p20b esd-container-frame">
                                      <table width="100%" cellspacing="0" cellpadding="0">
                                        <tbody>
                                          <tr>
                                            <td align="center" class="esd-block-image" style="font-size:0">
                                              <a target="_blank">
                                                <img src="https://elkeirh.stripocdn.email/content/guids/CABINET_8c3622c4a516db591e0c23764bc9a09a328d22f573d771eeff26d36d1ae64ae5/images/logo.jpg" alt="" width="135" class="adapt-img">
                                              </a>
                                            </td>
                                          </tr>
                                          <tr>
                                            <td align="center" class="esd-block-text">
                                              <p>
                                                <strong>Roi Et Technical College</strong>
                                              </p>
                                            </td>
                                          </tr>
                                        </tbody>
                                      </table>
                                    </td>
                                  </tr>
                                </tbody>
                              </table>
                              <!--[if mso]></td><td width="20"></td><td width="360" valign="top"><![endif]-->
                              <table cellspacing="0" cellpadding="0" align="right" class="es-right">
                                <tbody>
                                  <tr>
                                    <td width="360" align="left" class="esd-container-frame">
                                      <table width="100%" cellspacing="0" cellpadding="0">
                                        <tbody>
                                          <tr>
                                            <td align="center" bgcolor="#ffffff" class="esd-block-text es-text-5035 es-p10t">
                                              <p style="color:#0b5394;line-height:170%">
                                                <strong class="es-text-mobile-size-24" style="font-size:24px">Development of&nbsp;</strong>
                                              </p>
                                              <p style="color:#0b5394;line-height:170%">
                                                <strong class="es-text-mobile-size-24" style="font-size:24px">smart air purifiers</strong>
                                              </p>
                                              <h2 class="es-text-mobile-size-20" style="color:#f80404;font-size:20px;line-height:170%">
                                                <strong>Flammable gas detected !!!</strong>
                                              </h2>
                                            </td>
                                          </tr>
                                        </tbody>
                                      </table>
                                    </td>
                                  </tr>
                                </tbody>
                              </table>
                              <!--[if mso]></td></tr></table><![endif]-->
                            </td>
                          </tr>
                        </tbody>
                      </table>
                    </td>
                  </tr>
                </tbody>
              </table>
              <table cellspacing="0" cellpadding="0" align="center" class="es-content">
                <tbody>
                  <tr>
                    <td align="center" class="esd-stripe">
                      <table width="600" cellspacing="0" cellpadding="0" bgcolor="#ffffff" align="center" class="es-content-body">
                        <tbody>
                          <tr>
                            <td align="left" class="es-p20t es-p20r es-p20l esd-structure">
                              <table width="100%" cellspacing="0" cellpadding="0">
                                <tbody>
                                  <tr>
                                    <td width="560" valign="top" align="center" class="esd-container-frame">
                                      <table width="100%" cellspacing="0" cellpadding="0">
                                        <tbody>
                                          <tr>
                                            <td align="left" class="esd-block-text">
                                              <p>
                                                &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;This is a fire alarm from the "<span style="color:#241e1e">Development of smart air purifiers</span>" no {current_datetime} 
<br><strong>Detected gas levels exceeding the specified value:{gas over} </strong> 
<br>Please check your safety.
                                              </p>
                                            </td>
                                          </tr>
                                        </tbody>
                                      </table>
                                    </td>
                                  </tr>
                                </tbody>
                              </table>
                            </td>
                          </tr>
                        </tbody>
                      </table>
                    </td>
                  </tr>
                </tbody>
              </table>
              <table cellspacing="0" cellpadding="0" align="center" class="esd-footer-popover es-footer">
                <tbody>
                  <tr>
                    <td align="center" class="esd-stripe">
                      <table width="600" cellspacing="0" cellpadding="0" bgcolor="#ffffff" align="center" class="es-footer-body">
                        <tbody>
                          <tr>
                            <td align="left" class="esd-structure es-p20t es-p20b es-p20r es-p20l">
                              <!--[if mso]><table width="560" cellpadding="0" 
                        cellspacing="0"><tr><td width="270" valign="top"><![endif]-->
                              <table cellspacing="0" cellpadding="0" align="left" class="es-left">
                                <tbody>
                                  <tr>
                                    <td width="270" align="left" class="es-m-p20b esd-container-frame">
                                      <table width="100%" cellspacing="0" cellpadding="0">
                                        <tbody>
                                          <tr>
                                            <td align="center" class="esd-block-image" style="font-size:0">
                                              <a target="_blank" href="https://www.google.com/maps/place/%E0%B8%A7%E0%B8%B4%E0%B8%97%E0%B8%A2%E0%B8%B2%E0%B8%A5%E0%B8%B1%E0%B8%A2%E0%B9%80%E0%B8%97%E0%B8%84%E0%B8%99%E0%B8%B4%E0%B8%84%E0%B8%A3%E0%B9%89%E0%B8%AD%E0%B8%A2%E0%B9%80%E0%B8%AD%E0%B9%87%E0%B8%94/@16.0526845,103.6515979,16z/data=!4m6!3m5!1s0x3117fdcbc956ba0d:0x415e931848db9c6!8m2!3d16.0530351!4d103.655514!16s%2Fg%2F11ddzz3v9h?entry=ttu&g_ep=EgoyMDI0MTEwNi4wIKXMDSoASAFQAw%3D%3D">
                                                <img src="https://elkeirh.stripocdn.email/content/guids/CABINET_8c3622c4a516db591e0c23764bc9a09a328d22f573d771eeff26d36d1ae64ae5/images/map.PNG" alt="" width="190" class="adapt-img">
                                              </a>
                                            </td>
                                          </tr>
                                        </tbody>
                                      </table>
                                    </td>
                                  </tr>
                                </tbody>
                              </table>
                              <!--[if mso]></td><td width="20"></td><td width="270" valign="top"><![endif]-->
                              <table cellspacing="0" cellpadding="0" align="right" class="es-right">
                                <tbody>
                                  <tr>
                                    <td width="270" align="left" class="esd-container-frame">
                                      <table width="100%" cellspacing="0" cellpadding="0">
                                        <tbody>
                                          <tr>
                                            <td align="left" class="esd-block-text es-text-7676 es-p10t">
                                              <p>
                                                <strong class="es-text-mobile-size-11" style="font-size:11px">Project Smart Air Purifier:</strong><span class="es-text-mobile-size-11" style="font-size:11px"> An air cleaner that tracks information and evaluates the levels of PM dust, temperature, humidity, gas in your home. This is a student project created by electrical technology students at Roi Et Technical College in Thailand.</span>
                                              </p>
                                              <p>
                                                <span class="es-text-mobile-size-11" style="font-size:11px">
</span><span class="es-text-mobile-size-11" style="font-size:11px;color:#3d85c6"><a href="https://thingspeak.mathworks.com/channels/1976729" style="color:#3d85c6">Smart air purifier - ThingSpeak IoT</a></span><span class="es-text-mobile-size-11" style="font-size:11px">

</span>
                                              </p>
                                            </td>
                                          </tr>
                                        </tbody>
                                      </table>
                                    </td>
                                  </tr>
                                </tbody>
                              </table>
                              <!--[if mso]></td></tr></table><![endif]-->
                            </td>
                          </tr>
                        </tbody>
                      </table>
                    </td>
                  </tr>
                </tbody>
              </table>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
            """
            html_content = html_content.replace("{current_datetime}", current_datetime)     
        # Send the email notification
            send_email("Flammable Gas Detected - Urgent!", html_content)
 
            # Wait before checking again (set an interval as needed)
            time.sleep(120)  # Check every 60 seconds

    else:
        print(f"Gas level normal: {gas_level} (Checked at {datetime.now()})")

    # รอ 30 วินาทีก่อนตรวจสอบใหม่
    time.sleep(20)
