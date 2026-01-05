Emulator Code (Display + UI Testbench)
Doel van deze repository

Deze repository bevat emulator-/testcode die primair is opgezet om:

de display stack te testen zonder volledige target-hardware,

de UI-implementatie te valideren,

en specifiek de drie UI-schermen (UI 1/2/3) snel te kunnen ontwikkelen en debuggen.

Het project fungeert daarmee als testbench voor de grafische laag: rendering, schermovergangen, layout, events en (optioneel) mockdata.

Scope (wat je hier wel / niet verwacht)
Wel

UI-opbouw en schermlogica (3 UI’s)

Display-initialisatie en (emulatie-)driverlaag

Snelle iteratie op visuals/UX zonder de volledige embedded applicatie

Unit-/componenttests (GoogleTest) voor relevante logica (waar van toepassing)

Niet (of beperkt)

Volledige batterij-/SMU-logica zoals in het hoofdproject

Productie-ready hardware-integratie voor alle randapparatuur

End-to-end systeemgedrag met alle safety/meetsubsysteemdetails
