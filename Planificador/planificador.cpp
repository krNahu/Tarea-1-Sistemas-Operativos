#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <ctime>
#include <csignal>
#include <unistd.h>
#include <sys/wait.h>

using namespace std;

enum Estado { ESPERANDO, CORRIENDO, TERMINADA, FALLIDA, ABORTADA };

struct Actividad {
    string ID_Actividad;
    string Nombre_Actividad;
    int tiempo_ms;
    vector<string> Dependencias;
    vector<int> dependientes;
    int pendientes;
    int estado;
    string inbox;
    int fdLectura;
    pid_t pid;
};

vector<Actividad> acts;
map<string, int> indice;
vector<int> corriendo;
int terminadas = 0;
volatile sig_atomic_t seremi = 0;
sigset_t mascaraOriginal;

string trim(string s) {
    int ini = 0;
    int fin = s.size() - 1;
    while (ini <= fin && (s[ini] == ' ' || s[ini] == '\t' || s[ini] == '\r' || s[ini] == '\n')) {
        ini++;
    }
    while (fin >= ini && (s[fin] == ' ' || s[fin] == '\t' || s[fin] == '\r' || s[fin] == '\n')) {
        fin--;
    }
    return s.substr(ini, fin - ini + 1);
}

vector<string> cortar(string texto, char separador) {
    vector<string> partes;
    stringstream ss(texto);
    string parte;
    while (getline(ss, parte, separador)) {
        partes.push_back(trim(parte));
    }
    return partes;
}

bool esEnteroPositivo(string s, int &valor) {
    if (s == "") return false;
    for (int i = 0; i < (int)s.size(); i++) {
        if (!isdigit((unsigned char)s[i])) return false;
    }
    long v = atol(s.c_str());
    if (v <= 0 || v > 1000000000L) return false;
    valor = (int)v;
    return true;
}

bool leerPlan(string archivo) {
    ifstream f(archivo.c_str());
    if (!f.is_open()) {
        cerr << "no se pudo abrir " << archivo << endl;
        return false;
    }

    string linea;
    int nlinea = 0;
    while (getline(f, linea)) {
        nlinea++;
        if (trim(linea) == "") continue;

        vector<string> campos = cortar(linea, ':');
        if (campos.size() < 2 || campos[0] == "") {
            cerr << "linea " << nlinea << " mal formada" << endl;
            return false;
        }

        Actividad a;
        a.ID_Actividad = campos[0];
        a.Nombre_Actividad = campos[1];

        int t = 0;
        if (campos.size() >= 3 && campos[2] != "" && esEnteroPositivo(campos[2], t)) {
            a.tiempo_ms = t;
        } else {
            a.tiempo_ms = 100 + rand() % 4901;
        }

        if (campos.size() >= 4) {
            vector<string> deps = cortar(campos[3], ',');
            for (int j = 0; j < (int)deps.size(); j++) {
                if (deps[j] != "") a.Dependencias.push_back(deps[j]);
            }
        }

        a.pendientes = 0;
        a.estado = ESPERANDO;
        a.fdLectura = -1;
        a.pid = -1;

        if (indice.count(a.ID_Actividad) > 0) {
            cerr << "ID repetido: " << a.ID_Actividad << endl;
            return false;
        }
        indice[a.ID_Actividad] = acts.size();
        acts.push_back(a);
    }

    for (int i = 0; i < (int)acts.size(); i++) {
        for (int j = 0; j < (int)acts[i].Dependencias.size(); j++) {
            string dep = acts[i].Dependencias[j];
            if (indice.count(dep) == 0) {
                cerr << "la actividad " << acts[i].ID_Actividad << " depende de un id que no existe " << dep << endl;
                return false;
            }
            int p = indice[dep];
            acts[p].dependientes.push_back(i);
            acts[i].pendientes++;
        }
    }
    return true;
}

int main(){

return 0;
}
