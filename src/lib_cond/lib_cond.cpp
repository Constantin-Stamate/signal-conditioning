#include "lib_cond.h"

float lib_cond_saturate(
    float value,
    float minimum,
    float maximum
) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

Median5::Median5()
    : buffer{0, 0, 0, 0, 0},
      next_index(0),
      sample_count(0) {
}

float Median5::push(float value) {
    buffer[next_index] = value;
    next_index = (next_index + 1) % 5;

    if (sample_count < 5) {
        ++sample_count;
    }

    float sorted[5];

    for (unsigned char i = 0; i < sample_count; ++i) {
        sorted[i] = buffer[i];
    }

    for (unsigned char i = 0; i < sample_count; ++i) {
        for (unsigned char j = i + 1; j < sample_count; ++j) {
            if (sorted[j] < sorted[i]) {
                float temporary = sorted[i];
                sorted[i] = sorted[j];
                sorted[j] = temporary;
            }
        }
    }

    return sorted[sample_count / 2];
}

bool Median5::ready() const {
    return sample_count >= 5;
}

WeightedAvg4::WeightedAvg4()
    : buffer{0, 0, 0, 0},
      weights{50.0f, 25.0f, 15.0f, 10.0f},
      next_index(0),
      sample_count(0) {
}

float WeightedAvg4::push(float value) {
    buffer[next_index] = value;
    next_index = (next_index + 1) % 4;

    if (sample_count < 4) {
        ++sample_count;
    }

    float weighted_sum = 0.0f;
    float total_weight = 0.0f;

    for (unsigned char i = 0; i < sample_count; ++i) {
        int index =
            (static_cast<int>(next_index) - 1 - i + 4) % 4;

        weighted_sum += buffer[index] * weights[i];
        total_weight += weights[i];
    }

    if (total_weight == 0.0f) {
        return value;
    }

    return weighted_sum / total_weight;
}

bool WeightedAvg4::ready() const {
    return sample_count >= 4;
}