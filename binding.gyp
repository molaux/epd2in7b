{
   "targets" : [
      {
         "target_name": "epd2in7b",
         "sources": ["src/epd2in7b.cpp", "src/epdif.cpp"],
		 "libraries": [ "-L/usr/local/lib", "-lwiringPi"],
         "include_dirs" : [
            "<!(node -e \"require('nan')\")"
        ]
      }
   ]
}
