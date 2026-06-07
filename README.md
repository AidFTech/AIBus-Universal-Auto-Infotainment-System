# AidF AIBus Universal Car Audio System

***Disclaimer:*** *"AIBus" refers to the bus standard used by the devices in this project to communicate with each other. It is a play on words of AidF and BMW IBus. This project does not use or contain any material created by artificial intelligence.*

The goal of this project is to create a more or less universal car stereo system with the ability to retain all external OEM features, such as CD changers, Sirius tuners, and non-radio displays (e.g. audio screens on the cluster). It adds Apple CarPlay and Android Auto through a Carlinkit for music and navigation. At the heart of this system is the AIBus interface, based heavily on BMW's IBus, to allow all components to communicate with each other. The principal components, which can be used in any vehicle, are the Raspberry Pi navigation computer and the Arduino radio tuner. The screen is a separate module and may be vehicle-specific or universal. All vehicle-specific/OEM components communicate with the system via one or more translator modules.

This is an ongoing project and I am a relatively new embedded programmer- any tips would be appreciated!

Thanks to Ted and [Bluebus](https://github.com/tedsalmon/BlueBus.git) for the inspiration!

