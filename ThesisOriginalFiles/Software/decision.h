// Collection of functions that control whe our implementation is called
// The function must return false to indicate that our functions are to be run

bool BddSkip(uint counter,uint implementation)
{
    #ifdef OUTPUT_TEST_DATA
    if(implementation == TEST_TYPE && counter == TEST_COUNTER){
        return false;
    }
    else{
        return true;
    }
    #endif

    switch(implementation)
    {
        case BDD_AND_TAG:{

        }break;
        case BDD_EXIST_TAG:{

        }break;
        case BDD_ABSTRACT_TAG:{

        }break;
        case BDD_PERMUTE_TAG:{

        }break;
    }

    return false;
}
