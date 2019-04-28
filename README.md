# epigram

Includes Epigram: a type-smart, ergnomonic, fast key/value data structure for serialization, persistence, marshalling, named arguments, etc.

Some of the functionality is demonstrated below. Objects can also be serialized. 


```C++
#include <iostream>

#include "Epigram.hpp"
#include "EpAttribute.hpp"

d_epAttribute (double, args, smoothingCoeff);
d_epAttribute (int,    args, interpolationFactor);

using namespace std;

void Func (EpigramRef i_args)
{
    using namespace a_args;

    double smoothing = i_args [smoothingCoeff];        
        cout << "smoothing: " << smoothing;
        cout << ", interpolation: " << (int) i_args [interpolationFactor] << endl;
}

int main ()
{
    Epigram e;

    e ["named value"] = 1245.6;
    double namedValue = e ["named value"];          
        cout << "named value: " << namedValue << endl;

    double softExtraction = 888;
    e ["not here"] >> softExtraction;
        cout << "soft: " << softExtraction << endl;

    double exists = 6.5421;
    e ["named value"] >> exists;
        cout << "exists: " << exists << endl;

    e [999] = "string";
        cout << "[999]: " << e [999].To <string> () << endl;
	
    int array [] = { 1,2,3,4,5,6 };
    e ["epigramCanCast"] = array;
    vector <float> casted = e ["epigramCanCast"];
        cout << "casted vector: ";
        for (float f : casted)
            cout << f << " ";
        cout << endl;

    using namespace a_args;

    Func ({ smoothingCoeff= 3.14, interpolationFactor= 49 });
}
```

Output:
```
named value: 1245.6
soft: 888
exists: 1245.6
[999]: string
casted vector: 1 2 3 4 5 6
smoothing: 3.14, interpolation: 49
```
