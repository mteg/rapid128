#include <string.h>
#include <sys/types.h>
#include "read128.h"
struct r128_codetree_node * code_tree = 
 &(struct r128_codetree_node) {-1, 
  NULL,
  &(struct r128_codetree_node) {-1, 
   &(struct r128_codetree_node) {-1, 
    &(struct r128_codetree_node) {-1, 
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       NULL,
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {68, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {67, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {74, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {73, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {35, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {5, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {34, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {94, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {38, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {8, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {37, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {44, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {47, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {79, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        }
       }
      }
     },
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {66, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {4, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {3, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {82, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {65, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {81, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {72, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {7, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {6, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {14, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {71, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {13, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {17, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {16, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {85, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {84, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       }
      }
     }
    },
    &(struct r128_codetree_node) {-1, 
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {64, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {33, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {93, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {63, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {80, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       NULL,
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {92, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         }
        }
       }
      }
     },
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {70, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {36, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {43, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {69, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {12, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {42, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         }
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {46, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {15, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {45, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {99, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {96, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {83, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {95, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {100, NULL, NULL},
            NULL
           }
          }
         }
        },
        NULL
       }
      }
     }
    }
   },
   &(struct r128_codetree_node) {-1, 
    &(struct r128_codetree_node) {-1, 
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {75, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {78, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {41, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {11, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {40, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {50, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {32, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {106, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {61, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {10, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {9, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {20, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {76, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {19, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {2, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {1, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {18, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {22, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       }
      }
     },
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {103, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {39, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {49, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {104, NULL, NULL},
            NULL
           },
           NULL
          },
          NULL
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {105, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        }
       },
       NULL
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {31, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {0, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {30, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {89, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {52, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {21, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {51, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {53, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {90, NULL, NULL},
            NULL
           }
          }
         },
         NULL
        }
       }
      }
     }
    },
    &(struct r128_codetree_node) {-1, 
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        NULL,
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {56, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {59, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {26, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {25, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {29, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {28, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       }
      },
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {55, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {24, NULL, NULL},
            NULL
           },
           NULL
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {54, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {101, NULL, NULL},
            NULL
           }
          }
         }
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {58, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {27, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {57, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {23, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {48, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {60, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        }
       }
      }
     },
     &(struct r128_codetree_node) {-1, 
      &(struct r128_codetree_node) {-1, 
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         NULL,
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {62, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {88, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {87, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         NULL
        }
       },
       &(struct r128_codetree_node) {-1, 
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {98, NULL, NULL},
            NULL
           }
          },
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {86, NULL, NULL},
            NULL
           },
           NULL
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {97, NULL, NULL},
            NULL
           },
           NULL
          },
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {102, NULL, NULL},
            NULL
           }
          }
         }
        },
        &(struct r128_codetree_node) {-1, 
         &(struct r128_codetree_node) {-1, 
          NULL,
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {91, NULL, NULL},
            NULL
           }
          }
         },
         &(struct r128_codetree_node) {-1, 
          &(struct r128_codetree_node) {-1, 
           NULL,
           &(struct r128_codetree_node) {-1, 
            &(struct r128_codetree_node) {77, NULL, NULL},
            NULL
           }
          },
          NULL
         }
        }
       }
      },
      NULL
     }
    }
   }
  }
 };
