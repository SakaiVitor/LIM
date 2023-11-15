#include <iostream>
#include <cmath>

using namespace std;

class complexo{
    float Re;
    float Im;
public:
    float getRe(){return Re;}
    float getIm(){return Im;}
    void setRe(float Real){this->Re = Real;}
    void setIm(float Imaginario){this->Im = Imaginario;}
    complexo(float Real = 0, float Imaginaria = 0):Re(Real),Im(Imaginaria){}

    void print(){
        if(Im>=0)
            cout << Re << "+" << Im << "i" << endl;
        else 
            cout << Re << Im << "i" << endl; 
        };

    float modulo(){
        return pow((pow(Re,2.0) + pow(Im,2.0)),1.0/2.0);
    }

    complexo conjugado(){
        return complexo(Re,-Im);
    }

    complexo operator+(complexo const& obj)
    {
        complexo result;
        result.Re = Re + obj.Re;
        result.Im = Im + obj.Im;
        return result;
    }
    
    complexo operator-(complexo const& obj)
    {
        complexo result;
        result.Re = Re - obj.Re;
        result.Im = Im - obj.Im;
        return result;
    }

    complexo operator*(complexo const& obj)
    {
        complexo result;
        result.Re = Re * obj.Re - Im * obj.Im;
        result.Im = Im * obj.Re + Re * obj.Im;
        return result;
    }

    complexo operator/(const complexo& obj){
        double denominador = obj.Re * obj.Re + obj.Im * obj.Im;
        if (denominador != 0) {
            double real = (Re * obj.Re + Im * obj.Im) / denominador;
            double imag = (Im * obj.Re - Re * obj.Im) / denominador;
            return complexo(real, imag);
        } else {
            cout << "Divisão por zero!";
        }
    }

    bool operator==(const complexo& other) const {
        return (std::abs(Re - other.Re) < 1e-6) && (std::abs(Im - other.Im) < 1e-6);
    }

};

int main(){
    complexo complexo1 = complexo(3, -2);
    complexo1.print();
    complexo1.conjugado().print();
    cout << complexo1.modulo() << endl;
    complexo complexo2 = complexo(1, 10);
    (complexo1 + complexo2).print();
    (complexo1 - complexo2).print();
    (complexo1/complexo2).print();
    (complexo1*complexo2).print();
    complexo complexo3 = complexo();
    complexo3.print();
}