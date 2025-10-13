#include "quirc_internal.h"
#include <string.h>

#define FINGERPRINT_COUNT 2
static const struct quirc_point feature_points[FINGERPRINT_COUNT] = {
    {0, 8},
    {9, 4}
};

static const uint8_t fingerprint_3[FINGERPRINT_COUNT] = {1, 1};
static const uint8_t fingerprint_2[FINGERPRINT_COUNT] = {0, 1};
static const uint8_t fingerprint_1[FINGERPRINT_COUNT] = {0, 0};


/**
 * @brief  ʹ��������ȶԷ�������ʶ��һ���Ѷ�λ��QR�롣
 * @param  q      quirc���󣬱����� quirc_end() ���ú�ʹ�á�
 * @param  index  Ҫʶ���QR����quirc�ڲ������� (��0��ʼ)��
 * @return int    ����ʶ����: 1, 2, 3, ���� 0 (�������Ŀ����)��
 */
int fast_identify_qr(const struct quirc *q, int index) {
    if (index < 0 || index >= q->num_grids) {
        return 0;
    }
    const struct quirc_grid *qr = &q->grids[index];

    if (qr->grid_size != 21) {
        return 0;
    }
    uint8_t measured_fingerprint[FINGERPRINT_COUNT];
    for (int i = 0; i < FINGERPRINT_COUNT; i++) {
        struct quirc_point p;
        int x_mod = feature_points[i].x;
        int y_mod = feature_points[i].y;
        perspective_map(qr->c, (quirc_float_t)x_mod + 0.5, (quirc_float_t)y_mod + 0.5, &p);
        if (p.y < 0 || p.y >= q->h || p.x < 0 || p.x >= q->w) {
            return 0;
        }
        measured_fingerprint[i] = (q->pixels[p.y * q->w + p.x] == QUIRC_PIXEL_BLACK) ? 1 : 0;
    }

    if (memcmp(measured_fingerprint, fingerprint_1, sizeof(measured_fingerprint)) == 0) {
        return 1;
    }
    if (memcmp(measured_fingerprint, fingerprint_2, sizeof(measured_fingerprint)) == 0) {
        return 2;
    }
    if (memcmp(measured_fingerprint, fingerprint_3, sizeof(measured_fingerprint)) == 0) {
        return 3;
    }
    return 0;
}