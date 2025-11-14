#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <jack/jack.h>

#define NUMCOEF 129
#define RATE    4

// rcosdesign(0, 6, 16, 'normal')'

float coef[NUMCOEF] = {
  0.000000000000000,
  0.000028564660442,
  0.000032442735445,
  0.000013037751435,
  0.000000000000000,
  0.000014413368616,
  0.000039652232210,
  0.000038602538905,
  -0.000000000000000,
  -0.000042984467639,
  -0.000049168767941,
  -0.000019905538868,
  -0.000000000000000,
  -0.000022350642948,
  -0.000061995403055,
  -0.000060871214409,
  0.000000000000000,
  0.000069017613799,
  0.000079708375357,
  0.000032593296976,
  0.000000000000000,
  0.000037384349118,
  0.000104879441259,
  0.000104207345959,
  -0.000000000000000,
  -0.000121197963573,
  -0.000141895714645,
  -0.000058860130902,
  -0.000000000000000,
  -0.000069634810661,
  -0.000198654000503,
  -0.000200901309608,
  0.000000000000000,
  0.000242830328448,
  0.000290340462273,
  0.000123159749764,
  0.000000000000000,
  0.000153062023433,
  0.000448707987150,
  0.000467239045773,
  -0.000000000000000,
  -0.000602908252555,
  -0.000747846645249,
  -0.000330150925838,
  -0.000000000000000,
  -0.000449518995135,
  -0.001388858055463,
  -0.001532797774595,
  0.000000000000000,
  0.002269127423252,
  0.003055487722018,
  0.001481446462096,
  0.000000000000000,
  0.002551166245115,
  0.009166463166054,
  0.012160195678452,
  -0.000000000000000,
  -0.030795300744131,
  -0.064165242162379,
  -0.065479933624630,
  0.000000000000000,
  0.140314143481351,
  0.320826210811895,
  0.474247631459618,
  0.534522486798813,
  0.474247631459618,
  0.320826210811895,
  0.140314143481351,
  0.000000000000000,
  -0.065479933624630,
  -0.064165242162379,
  -0.030795300744131,
  -0.000000000000000,
  0.012160195678452,
  0.009166463166054,
  0.002551166245115,
  0.000000000000000,
  0.001481446462096,
  0.003055487722018,
  0.002269127423252,
  0.000000000000000,
  -0.001532797774595,
  -0.001388858055463,
  -0.000449518995135,
  -0.000000000000000,
  -0.000330150925838,
  -0.000747846645249,
  -0.000602908252555,
  -0.000000000000000,
  0.000467239045773,
  0.000448707987150,
  0.000153062023433,
  0.000000000000000,
  0.000123159749764,
  0.000290340462273,
  0.000242830328448,
  0.000000000000000,
  -0.000200901309608,
  -0.000198654000503,
  -0.000069634810661,
  -0.000000000000000,
  -0.000058860130902,
  -0.000141895714645,
  -0.000121197963573,
  -0.000000000000000,
  0.000104207345959,
  0.000104879441259,
  0.000037384349118,
  0.000000000000000,
  0.000032593296976,
  0.000079708375357,
  0.000069017613799,
  0.000000000000000,
  -0.000060871214409,
  -0.000061995403055,
  -0.000022350642948,
  -0.000000000000000,
  -0.000019905538868,
  -0.000049168767941,
  -0.000042984467639,
  -0.000000000000000,
  0.000038602538905,
  0.000039652232210,
  0.000014413368616,
  0.000000000000000,
  0.000013037751435,
  0.000032442735445,
  0.000028564660442,
   0.000000000000000
};

int nextSymbol() {
  return rand() % 4;
}

static int phase = 0;

void nextSample(float *Isample, float *Qsample) {
  phase = (phase + 1) % RATE;
  if (phase == 0) {
    int sym = nextSymbol();
    *Isample = (sym & 1) ? 1.f : -1.f;
    *Qsample = (sym & 2) ? 1.f : -1.f;
  } else {
    *Isample = 0.0f;
    *Qsample = 0.0f;
  }
}

// traditional implementation
float tapsI[NUMCOEF];
float tapsQ[NUMCOEF];

float shapeSample(float sample, float *taps) {
  taps[0] = sample;
  
  float q = 0.0;
  int i;
  for (i = 0; i<NUMCOEF; i++)
    q += taps[i] * coef[i];
  
  for (i = NUMCOEF-1; i>0; i--)
    taps[i] = taps[i-1];
  
  return q;
}

float qpsk() {
  // compute complex baseband signals,
  // and modulate
  float Isample, Qsample;
  nextSample(&Isample, &Qsample);
  float Ibb, Qbb;
  Ibb = shapeSample(Isample, tapsI);
  Qbb = shapeSample(Isample, tapsQ);
  // now, modulate with a carrier fc = fs/4
  switch (phase) {
  case 0 : return Ibb;
  case 1 : return Qbb;
  case 2 : return -Ibb;
  case 3 : return -Qbb;
  }
  return 0; // unexpected
}

jack_port_t *input_port_left;
jack_port_t *output_port_left;
jack_port_t *input_port_right;
jack_port_t *output_port_right;
jack_client_t *client;

int process (jack_nframes_t nframes, void *arg) {
  jack_default_audio_sample_t *outleft, *outright;
  outleft  = jack_port_get_buffer (output_port_left, nframes);
  outright = jack_port_get_buffer (output_port_right, nframes);
  for (int i=0; i<nframes; i++) {
    outleft[i] = qpsk();
    outright[i] = (phase == 0) ? 3.0 : -3.0; 
  }
  return 0;
}

void jack_shutdown (void *arg) {
        exit (1);
}

int main (int argc, char *argv[]) {
  const char **ports;
  const char *client_name = "simple";
  const char *server_name = NULL;
  jack_options_t options = JackNullOption;
  jack_status_t status;

  client = jack_client_open (client_name, options, &status, server_name);
  if (client == NULL) {
    fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
    if (status & JackServerFailed) {
      fprintf (stderr, "Unable to connect to JACK server\n");
    }
    exit (1);
  }
  if (status & JackServerStarted) {
    fprintf (stderr, "JACK server started\n");
  }
  if (status & JackNameNotUnique) {
    client_name = jack_get_client_name(client);
    fprintf (stderr, "unique name `%s' assigned\n", client_name);
  }

  jack_set_process_callback (client, process, 0);
  jack_on_shutdown (client, jack_shutdown, 0);

  printf ("engine sample rate: %" PRIu32 "\n", jack_get_sample_rate (client));

  input_port_left = jack_port_register (client, "input_left",
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsInput, 0);
  output_port_left = jack_port_register (client, "output_left",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0);
  input_port_right = jack_port_register (client, "input_right",
                                   JACK_DEFAULT_AUDIO_TYPE,
                                   JackPortIsInput, 0);
  output_port_right = jack_port_register (client, "output_right",
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    JackPortIsOutput, 0);

  if ((input_port_left == NULL) || (output_port_left == NULL) ||
      (input_port_right == NULL) || (output_port_right == NULL)) {
    fprintf(stderr, "no more JACK ports available\n");
    exit (1);
  }

  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    exit (1);
  }

  while (1)
    sleep(10);

  jack_client_close (client);
  
  exit (0);
}
