# buoys

Scientific project (partial) for the Far Eastern Federal University (FEFU) in Vladivostok, Russia.

Main purpose:
Utilize the S2C Underwater Acoustic Modem/S2C USBL Underwater Communication Device to transmit data from a buoy to a receiver on the shore. 
A buoy will be deployed in the water and will be transmitting data to a receiver on the shore. The receiver will be connected to a computer running a program that is able to read the data and display it on a GUI. The GUI displays the data on graphs and saves the data to a SQL database. The GUI will also be able to display the location of the buoy on a map.

Features:
- Work with multiple buoys.
- Automatically request data from the buoys periodically.
- The buoy will be able to transmit its location to the receiver and the receiver's GUI will be able to display the location on a map.
- Send commands to the buoy from the GUI to change the buoy’s settings.
- Store the data in a SQL database.
- universal communication protocol for all buoys and sensors.