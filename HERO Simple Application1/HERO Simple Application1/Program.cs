using System;
using System.IO.Ports;
using Microsoft.SPOT;
using Microsoft.SPOT.Hardware;
using CTRE.Phoenix;
using CTRE.Phoenix.Controller;
using CTRE.Phoenix.MotorControl;
using CTRE.Phoenix.MotorControl.CAN;

namespace HERO_XInput_Gampad_Example
{
    public class Program
    {
        GameController _gamepad = new GameController(UsbHostDevice.GetInstance(0)); //creating a controller object
        static PneumaticControlModule _pcm = new CTRE.Phoenix.PneumaticControlModule(0); //creating a PCM object
        SafeOutputPort digitalOut1 = new SafeOutputPort(CTRE.HERO.IO.Port1.Pin3, false);  //Port1, PIN 4 on Hero Board > IN2 in L298 Connected to Linear Actuator
        SafeOutputPort digitalOut2 = new SafeOutputPort(CTRE.HERO.IO.Port1.Pin6, false);   //Port1, PIN 5 on Hero Board > IN1 in L298 Connected to Linear Actuator
        InputPort inputDead = new InputPort(CTRE.HERO.IO.Port6.Pin6, false, Port.ResistorMode.PullDown); //Deadman switch, Port6 Pin4 on Hero Board
        //InputPort inputPressure = new InputPort(CTRE.HERO.IO.Port1.Pin4, false, Port.ResistorMode.Disabled); //Input Pressure from Pi Port 26 connected to Port6 Pin5
        AnalogInput analogPressure = new AnalogInput(CTRE.HERO.IO.Port1.Analog_Pin4);  //Analong input pressure from Port1 Pin 4
        TalonSRX tal1 = new TalonSRX(1); //first Talon, ID = 1
        TalonSRX tal2 = new TalonSRX(2);//second Talon, ID = 2
        long solenoidPeriod; //Time to auto-shutdown solenoid valve after shooting/opening it.
        
        SerialPort arduinoComm = new SerialPort(CTRE.HERO.IO.Port6.UART, 9600);
        static byte[] arduinoCommBuffer = new byte[2];
        bool sentToArduino = false;

        /// <summary>
        /// The maximum pressure that should be allowed. 
        /// </summary>
        double Pressure_Threshold_Value = 0.51;

        /// <summary>
        /// Message to send to the arduino to allow firing.
        /// </summary>
        readonly byte[] arduinoSetupFire = new byte[2] { (byte)'F', 0x2 };

        /// <summary>
        /// The uart timeout if not able to send or receive by the require time in milliseconds.
        /// </summary>
        readonly int uartTimeoutInMs = 100;

        /// <summary>
        /// The value if the Y button is pressed to fire the projectile.
        /// </summary>
        bool FIRE = false;

        /// <summary>
        /// The value of the deadman switch to allow firing or not.
        /// </summary>
        bool Deadman_Switch = false;


        // Actuator active for only 5 seconds.
        bool actuatorOn;
        long actuatorTimeout;
        long actuatorTime;


        public void RunForever()
        {
            _pcm.SetSolenoidOutput(1, false); //Initialize the compressor to be turned off, compressor ID = 1
            bool SolenoidTimer = false;
            tal1.ConfigFactoryDefault();
            tal2.ConfigFactoryDefault();

            if(!arduinoComm.IsOpen)
            {
                arduinoComm.ReadTimeout = arduinoComm.WriteTimeout = uartTimeoutInMs;
                arduinoComm.Open();
            }

            int i = 0;
            while (true)
            {

                if (_gamepad.GetConnectionStatus() == UsbDeviceConnection.Connected)
                {

                    CTRE.Phoenix.Watchdog.Feed();
                }
                else
                {
                    Debug.Print("Not connected: " + i); //The controller is not connected
                }
                i++;


                //Linear Actuator
                bool Stopmovement = _gamepad.GetButton(1); //X-Button
                bool Extend = _gamepad.GetButton(2);//A-Button
                bool Retract = _gamepad.GetButton(3); //B-Button
                
                if (Extend)
                {
                    digitalOut1.Write(false);
                    digitalOut2.Write(true);
                    actuatorOn = true;
                    actuatorTimeout = (5 * TimeSpan.TicksPerSecond) + DateTime.Now.Ticks;
                    actuatorTime = DateTime.Now.Ticks;

                }
                if (Retract)
                {
                    digitalOut1.Write(true);
                    digitalOut2.Write(false);
                    actuatorOn = true;
                    actuatorTimeout = (5 * TimeSpan.TicksPerSecond) + DateTime.Now.Ticks;
                    actuatorTime = DateTime.Now.Ticks;
                }
                if (Stopmovement)
                {
                    digitalOut1.Write(false);
                    digitalOut2.Write(false);
                    actuatorOn = false;
                }

                if(actuatorOn)
                {
                    actuatorTime = DateTime.Now.Ticks;
                    if (actuatorTime > actuatorTimeout)
                    {
                        digitalOut1.Write(false);
                        digitalOut2.Write(false);
                        actuatorOn = false;
                    }
                }
                

                //Pressure Sensor
                //Boolean Pressure_Switch = inputPressure.Read(); //Input from Raspberry Pi
                double pressure_value = analogPressure.Read();  // Pressure Value read from Analog Input (Port 1 Pin 4)
                string pressure_string = pressure_value.ToString(); //Converted to string for debugging
                String pressure = "Under threshold";
              /*  if (Pressure_Switch)
                {
                    pressure = "Above threshold";
                    _pcm.SetSolenoidOutput(1, false);	//Pressure is above threshold, turn off solenoid
                }*/
                Debug.Print("Pressure Value: " + pressure); //Print pressure value

                //Compressor
                Boolean StartCompressor = _gamepad.GetButton(10); //"START"-Button
                Boolean StopCompressor = _gamepad.GetButton(9); //"BACK"-Button
                

                if (StartCompressor)//&& (!Pressure_Switch)) //If pressure is below threshold and "START" is pressed
                {
                    // Stop the compressor if over the threshold.
                    if (pressure_value > Pressure_Threshold_Value)
                    {
                        _pcm.SetSolenoidOutput(1, false);
                        Debug.Print("Pressure is too damn high!");
                    }

                    else
                    {

                        if (sentToArduino)
                        {
                            sentToArduino = false;
                        }

                        _pcm.SetSolenoidOutput(1, true); //Start compressor
                        Debug.Print("StartCompressor");
                        Debug.Print(pressure_string);
                    }

                }

                // When the start button is released...
                else
                {
                    _pcm.SetSolenoidOutput(1, false); //Stop compressor
                }





                FIRE = _gamepad.GetButton(4); //Y-Button
                Deadman_Switch = inputDead.Read();
                if (FIRE && (Deadman_Switch)) //If Y and the deadman switch is pressed... 
                {
                    if(!sentToArduino)
                    {
                        arduinoComm.Write(arduinoSetupFire, 0, 2);
                        sentToArduino = true;
                    }

                    // This will serve as a timer and ready signal from the Arduino board.
                    // It is a timer since there is a read timeout set and if no bytes are read
                    // by the timeout the program will continue. 
                    // If there are bytes successfully read from the Arduino, the program will
                    // also continue immediately. 
                    int bRead = arduinoComm.Read(arduinoCommBuffer, 0, arduinoCommBuffer.Length);


                    _pcm.SetSolenoidOutput(0, true); //Open Solenoid/Fire ID = 0 for solenoid
                    solenoidPeriod = (500 * TimeSpan.TicksPerMillisecond) + DateTime.Now.Ticks; //Start timer for half a second
                    SolenoidTimer = true;
                }
                if (SolenoidTimer)
                {
                    long nowSolenoid = DateTime.Now.Ticks;
                    if (nowSolenoid > solenoidPeriod) //If half a second has passed
                    {
                        _pcm.SetSolenoidOutput(0, false); //Close the solenoid
                        SolenoidTimer = false;
                        Debug.Print("Close Solenoid");
                    }
                }



                bool LeftForward = _gamepad.GetButton(5); //LB
                bool LeftBackward = _gamepad.GetButton(7); //LT
                bool RightForward = _gamepad.GetButton(6); //RB
                bool RightBackward = _gamepad.GetButton(8); //RT

                float LeftY = 0;
                float RightY = 0;
                //Talon SRX == Drive System
                if (LeftForward)
                {
                    LeftY = (float)(0.5);
                }
                if (LeftBackward)
                {
                    LeftY = (float)(-0.5);

                }
                if (RightForward)
                {
                    RightY = (float)(0.5);
                }
                if (RightBackward)
                {
                    RightY = (float)(-0.5);
                }


                tal1.Set(ControlMode.PercentOutput, LeftY * -1); //moving tal1 with LT & LB
                tal2.Set(ControlMode.PercentOutput, RightY); //moving tal2 with RT & RB

                System.Threading.Thread.Sleep(10);
            }
        }




        public static void Main()
        {
            new Program().RunForever();
        }
    }
}
