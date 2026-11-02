#pragma once

class Median5 {
public:
    Median5();

    float push(float value);
    bool ready() const;

private:
    float buffer[5];
    unsigned char next_index;
    unsigned char sample_count;
};

class WeightedAvg4 {
public:
    WeightedAvg4();

    float push(float value);
    bool ready() const;

private:
    float buffer[4];
    float weights[4];
    unsigned char next_index;
    unsigned char sample_count;
};

float lib_cond_saturate(
    float value,
    float minimum,
    float maximum
);