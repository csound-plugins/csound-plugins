function(configure_portable_audio_plugins)
  target_link_libraries(rtpa PRIVATE ALSA::ALSA Jack::jack)
  target_link_libraries(pmidi PRIVATE ALSA::ALSA)
endfunction()

# This file is loaded immediately after Csound's top-level project() call.
# Defer target configuration until its plugin subdirectories have been added.
cmake_language(DEFER CALL configure_portable_audio_plugins)
