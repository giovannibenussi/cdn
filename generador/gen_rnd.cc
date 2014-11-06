#include "gen_rnd.h"
#include "../Client.h"
#include "../Constants.h"
#include <cmath>
// #include "../applicationLayer/TlcProtocol.h"
// #include "../applicationLayer/Query.h"


void Gen_rnd::inner_body( ) {
    // handle<TlcProtocol> *gq;
    string line, terms, line_querys_sended;
    char * ptr;
    // Query *q; // ============================
    int id = 0;
    int chosen = 0;
    int envP2P = 0;

    endStream.open( traces_file );
    //La primera linea se tira si se usa el Log de Yahoo
    getline( endStream, line );


    double phase_time = this->time();


    //cout<<"Linea "<<linea<<endl;
    double accumulated_time = time();
    double query_in_interval = 0;
    int num_cycles = 0;
    while ( 1 ) {
        if (time() - accumulated_time > 10) {
            (*chart_file) << time() << " " << query_in_interval << endl;
            query_in_interval = 0;
            accumulated_time = time();

            (*querys_sended_stream) << num_cycles;
            for (int i = 0; i < NUM_CLIENTS; ++i) {
                (*querys_sended_stream) << " " << clients[ i ]->GetNumberOfQuerysSendedThisCycle();
                clients[ i ]->ResetCycle();
            }
            (*querys_sended_stream) << endl;
            num_cycles++;
        }
        if ( ! getline( endStream, line ) ) {
            passivate();
        }
        // cout << ".";

        prev = actual;
        actual = this->time();
        //     double time = actual - prev;

        tokens.clear();
        util->Tokenize( line, tokens, "\t");

        terms = tokens[2];

        tokens.clear();
        util->Tokenize( terms, tokens, " ");

        ptr = util->obtain_terms( tokens );

        //  gq = queue_thread.front();
        //  queue_thread.pop_front();

        //  Verifica si va a un peer o al WSE, cuando la consulta llega desde un nodo fuera
        //  de la red de peers


        // ===== ELIJO UN CLIENTE AL AZAR Y ENVIO UN MENSAJE

        int id_client = random_client->value();
        int * hashValue = new int(1);
        MessageWSE * wseQuery = new MessageWSE(NULL, hashValue, ptr, USER);
        clients[ id_client ]->AddMessageWse(wseQuery);
        clients[ id_client ]->activate();

        // ||||| ELIJO UN CLIENTE AL AZAR Y ENVIO UN MENSAJE



        double prob = SelectSource->value();
        if (prob > porcentaje_peers) { //viene fuera de la red peer
            // int *hashValue = h->GenerateKey(ptr);
            int * hashValue = new int(1);
            MessageWSE * wseQuery = new MessageWSE(NULL, hashValue, ptr, USER);

            delete [] ptr;

            (*wse)->add_request(wseQuery);
            if ((*wse)->idle() && !(*wse)->get_busy())
                (*wse)->activateAfter(current());
        } else { // la consulta llega a un peer
            envP2P = 1;
            // gq = Peers[0]; // ============================

            switch ((int)Peer_Selection) {
                case 0: {
                    //RANDOM SELECTION -> UNIFORM
                    int rnd = rand() % NP;
                    // gq = Peers[rnd]; // ============================
                    ends[rnd]++;
                    break;
                }
                case 1: {
                    //ZIPF SELECTION -> CLUSTERING
                    rand_val(1);
                    int zipf = getZipf(1.0, NP - 1);
                    //cout << "Peer " << zipf <<endl;
                    fflush(stdout);
                    // gq = Peers[zipf]; // ============================
                    ends[zipf]++;
                    break;
                }
                case 2: {
                    // STATIC DEBUG PROPOSE
                    // gq = Peers[chosen]; // ============================
                    ends[chosen]++;
                    chosen++;
                    if (chosen >= NP)
                        chosen = 0;
                    break;
                }
            }

            // int *hashValue = h->GenerateKey(ptr);
            // cout << " Saliendo Query " << sentQueries << "-" <<id << endl;
            // q = new Query(sentQueries, ptr, hashValue,  this->time()); // ==================
            // cout << "debug1" << endl;
            (*observer)->addNQueriesOut();
            // cout << "debug2" << endl;
            delete [] ptr;
            //  cout << "debug3" << endl;
            //cout << id << " : " << q->term << endl;

            // (*gq)->add_query( q ); // ============================
            //  cout << "debug5" << endl;
            /*If node is sleeping it weaks it up*/

            // if ( (*gq)->idle()) { // && !(*gq)->get_busy() ) // ============================
            //     //   cout << "debug6" << endl;
            //     (*gq)->activateAfter( current() );
            // }
            // cout << "debug7" << endl;
        }//else

        ///CTE = arrival_time->value();
        sentQueries++;
        query_in_interval++;

        //cout<<"Generador envia "<<sentQueries<<endl;
        if (sentQueries >= (*totalQueries) ) {
            ends[NP] = 1; //Indica que se ha enviado la ultima consulta
            //cout<<"ULTIMA CONSULTA "<<endl;
            if ( envP2P == 0 ) { //No se enviaron consultas a los peers
                //Apaga replicadores
                // for ( int i = 0 ; i < (int)Peers.size(); i++) { // ============================
                //     gq = Peers[i];
                //     (*gq)->EndReplicator();
                // }
            }

            // (*observer)->end();
            passivate();

        }

        if (FLASH_CROWD) {

            double elapsed_time = this->time() - phase_time;
            //    cout << "elapsed: " << elapsed_time << endl;
            if (!phase) {
                if (elapsed_time > NORMAL_TIME) {
                    // cout << "change to crowd " << this->time() << ", processed: " << sentQueries << endl;
                    phase = true;
                    phase_time = this->time();
                    double newRate = (1.0 / CROWDED_RATE);
                    delete arrival_time;
                    arrival_time = new rngExp("Arrive Time", newRate);
                    arrival_time->reset();
                }
            } else {
                if (elapsed_time > CROWD_TIME) {
                    // cout << "change to normal" << this->time() << ", processed: " << sentQueries << endl;
                    phase = false;
                    phase_time = this->time();
                    double newRate = (1.0 / NORMAL_RATE);
                    delete arrival_time;
                    arrival_time = new rngExp("Arrive Time", newRate);
                    arrival_time->reset();
                }
            }


        }
        CTE = arrival_time->value();
        hold( CTE );

        id++;
    }
}

int Gen_rnd::getZipf (int alpha, int n) {

    static bool first = true;      // Static first time flag
    static double c = 0;          // Normalization constant
    double z;                     // Uniform random number (0 < z < 1)
    double sum_prob;              // Sum of probabilities
    double zipf_value = 0;          // Computed exponential value to be returne
    int i;                        // Loop counter

    // Compute normalization constant on first call only
    if (first == true) {
        for (i = 1; i <= n; i++)
            c = c + (1.0 / pow((double) i, alpha));
        c = 1.0 / c;
        first = false;
        //cout << "PRIMERO" << endl;
    }

    // Pull a uniform random number (0 < z < 1)
    do {
        z = (((double) rand()) / (RAND_MAX + 1.0));
        // cout << "Z VALUE " << z << endl;
    } while ((z == 0) || (z == 1));
    // Map z to the value
    sum_prob = 0;
    for (i = 1; i <= n; i++) {
        //      cout <<"Z " << z << endl;
        //     cout << "SUM PROB: " << sum_prob << endl;
        //      cout << "C " << c <<endl;
        //      cout << "ALPHA " << alpha << endl;
        sum_prob = sum_prob + (c / pow((double) i, alpha));
        if (sum_prob >= z) {
            zipf_value = i;
            break;
        }
    }
    //   cout << "ZIPF VALUE " << zipf_value << endl;
    // Assert that zipf_value is between 1 and N
    // ASSERT((zipf_value >= 1) && (zipf_value <= n)); // ===================
    return zipf_value ;
}


void Gen_rnd::setQueryRate( int newRate) {
    // newRate queries/sec
    switch (QUERY_RATE_STRATEGY) {
        // DELTA T TIME
        case 0:
            if (this->time() > QUERY_DELTA_T ) {
                double lambda = (1.0 / newRate);
                delete arrival_time;
                arrival_time = new rngExp("Arrive Time", lambda);
                arrival_time ->reset();
            }
            break;

        // Q QUERIES
        case 1:
            if ( (lastStepQueries + QUERY_DELTA_Q) < sentQueries) {
                lastStepQueries = sentQueries;
                double lambda = (1.0 / newRate);
                delete arrival_time;
                arrival_time = new rngExp("Arrive Time", lambda);
                arrival_time ->reset();
            }
            break;
    }
}


double Gen_rnd::rand_val(int seed) {
    const long a = 16807; // Multiplier
    const long m = 2147483647;// Modulus
    const long q = 127773;// m div a
    const long r = 2836;// m mod a
    static long x; // Random int value
    long x_div_q; // x divided by q
    long x_mod_q; // x modulo q
    long x_new; // New x value

    // Set the seed if argument is non-zero and then return zero
    if (seed > 0) {
        x = seed;
        return (0.0);
    }
    //
    // // RNG using integer arithmetic
    x_div_q = x / q;
    x_mod_q = x % q;
    x_new = (a * x_mod_q) - (r * x_div_q);
    if (x_new > 0)
        x = x_new;
    else
        x = x_new + m;
    //
    // // Return a random value between 0.0 and 1.0
    return ((double) x / m);
    // }


}
