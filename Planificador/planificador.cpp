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


void resumen() {
    int ok = 0, falladas = 0, abortadas = 0;
    for (int i = 0; i < (int)acts.size(); i++) {
        if (acts[i].estado == TERMINADA) ok++;
        if (acts[i].estado == FALLIDA) falladas++;
        if (acts[i].estado == ABORTADA) abortadas++;
    }
    cout << "\n Resumen" << endl;
    cout << "Terminadas: " << ok << endl;
    cout << "Falladas:   " << falladas << endl;
    cout << "Abortadas:  " << abortadas << endl;
}


void manejador(int sig) {
    (void)sig;
    seremi = 1;
}

void manejadorHijo(int sig) {
    (void)sig;
}

void abortarTodo() {
    cout << "\n Llego el SEREMI! Guarden todo :,v" << endl;

    for (int k = 0; k < (int)corriendo.size(); k++) {
        kill(acts[corriendo[k]].pid, SIGKILL);
    }
    for (int k = 0; k < (int)corriendo.size(); k++) {
        waitpid(acts[corriendo[k]].pid, NULL, 0);
        close(acts[corriendo[k]].fdLectura);
    }
    corriendo.clear();

    for (int i = 0; i < (int)acts.size(); i++) {
        if (acts[i].estado == ESPERANDO || acts[i].estado == CORRIENDO) {
            acts[i].estado = ABORTADA;
        }
    }
    resumen();
    exit(1);
}

void abortarRama(int origen) {
    vector<int> pila;
    pila.push_back(origen);

    while (!pila.empty()) {
        int x = pila.back();
        pila.pop_back();
        for (int k = 0; k < (int)acts[x].dependientes.size(); k++) {
            int h = acts[x].dependientes[k];
            if (acts[h].estado == ESPERANDO) {
                acts[h].estado = ABORTADA;
                terminadas++;
                cout << "[ABORTADO] " << acts[h].Nombre_Actividad << " (dependia de una actividad fallida)" << endl;
                pila.push_back(h);
            }
        }
    }
}

void fallaAlLanzar(int i) {
    acts[i].estado = FALLIDA;
    terminadas++;
    cout << "[FALLO] " << acts[i].Nombre_Actividad << " (no se pudo crear el proceso)" << endl;
    abortarRama(i);
}

void codigoHijo(int i, int fdEntrada, int fdSalida) {
    signal(SIGINT, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    sigprocmask(SIG_SETMASK, &mascaraOriginal, NULL);

    for (int k = 0; k < (int)corriendo.size(); k++) {
        close(acts[corriendo[k]].fdLectura);
    }

    char buffer[4096];
    int insumos = 0;
    int n;

    while ((n = read(fdEntrada, buffer, sizeof(buffer))) > 0) {
        for (int k = 0; k < n; k++) {
            if (buffer[k] == '\n') insumos++;
        }
    }
    close(fdEntrada);

    sleep(acts[i].tiempo_ms / 1000);
    usleep((acts[i].tiempo_ms % 1000) * 1000);

    if (acts[i].Nombre_Actividad.substr(0, 5) == "falla") {
        _exit(1);
    }

    char msg[256];
    snprintf(msg, 256, "%s terminada (recibio %d insumos)", acts[i].Nombre_Actividad.c_str(), insumos);
    if (write(fdSalida, msg, strlen(msg)) < 0) {
        _exit(1);
    }
    close(fdSalida);
    _exit(0);
}

void lanzar(int i) {
    int entrada[2];   
    int salida[2];  

    if (pipe(entrada) < 0) {
        perror("pipe");
        fallaAlLanzar(i);
        return;
    }
    if (pipe(salida) < 0) {
        perror("pipe");
        close(entrada[0]);
        close(entrada[1]);
        fallaAlLanzar(i);
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(entrada[0]); close(entrada[1]);
        close(salida[0]); close(salida[1]);
        fallaAlLanzar(i);
        return;
    }

    if (pid == 0) {
        close(entrada[1]);
        close(salida[0]);
        codigoHijo(i, entrada[0], salida[1]);
    }

    close(entrada[0]);
    close(salida[1]);

    if (write(entrada[1], acts[i].inbox.c_str(), acts[i].inbox.size()) < 0) {
    }
    close(entrada[1]); 

    acts[i].fdLectura = salida[0];
    acts[i].pid = pid;
    acts[i].estado = CORRIENDO;
    corriendo.push_back(i);
    cout << "[INICIA] " << acts[i].Nombre_Actividad << " (" << acts[i].tiempo_ms << " ms)" << endl;
}








int main(){

return 0;
}
