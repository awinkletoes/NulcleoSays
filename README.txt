Nucleo Says
Hayley Jamiola and Annaliese Winklosky
----------------------------------------------------------------------------
Wired Connections:

1) Nucleo Board conncected to Host PC via USB
2) Nucleo Board -> HM-10 Bluetooth Module
GND -> GND
3.3V -> VCC
PA10 (RX) -> TX
PA9 (TX) -> RX

3) Nucleo Board -> Wii Nunchuck
GND -> GND
3.3V -> VCC
PB9 -> Data (d)
PB8 -> Clock (c)

4) Nucleo Board -> LCD Display
GND -> GND
3.3V -> VCC
PA6 -> MISO
PA7 -> MOSI
PB6 -> CS
PC7 -> RST
PA8 -> Data or Command
PB3 -> SCK
----------------------------------------------------------------------------
How to Run Nucleo Says:
1) Open STMCubeIDE
2) Open the NucleoSays project
3) Clean the NucleoSays project
4) Build the NucleoSays project
5) Run the configuration for the NucleoSays project
6) Open a terminal in STMCubeIDE with the port the Nucleo board is connected to (BAUD: 115200)
7) Open BLESerial Tiny app on an iPhone
8) Connect to the "NucleoSays" BT module
9) Press the blue pushbutton on the board to start the game
10) Play the game.
----------------------------------------------------------------------------
Rules of the Game:
After pushing the blue pushbutton on the board to start the game, you will start with three lives. 
Each time you get an action wrong, you will lose a life. An action from the list below will be communicated 
to the iPhone connected to the BT module. Perform that action to increase the score. When you lose all three lives,
the game will end.
----------------------------------------------------------------------------
Action Name -> Action to Perform
C-It! -> Press the c button on the Nunchuck
Z-It! -> Press the z button on the Nunchuck
Push-It! -> Press the blue pushbutton on the Nucleo board
Twist-It! -> Move the Nunchuck's joystick around
Shake-It! -> Shake the Nunchuck in your hand
