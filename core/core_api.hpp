#pragma once

// На Windows нужно различать экспорт и импорт

#ifdef _WIN32
  #ifdef CORE_EXPORTS
    #define CORE_API __declspec(dllexport)
  #else
    #define CORE_API __declspec(dllimport)
  #endif
#else
  #define CORE_API
#endif