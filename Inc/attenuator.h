#ifndef ATTENUATOR_H
#define ATTENUATOR_H

#define ATTENUATOR_MAX_DB   31.5f
#define ATTENUATOR_MIN_DB   0.0f

void attenuator_init(void);
void attenuator_set(float attenuation_db);
float attenuator_get(void);

#endif // ATTENUATOR_H
