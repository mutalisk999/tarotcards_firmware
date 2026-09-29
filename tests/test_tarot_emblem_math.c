// tests/test_tarot_emblem_math.c —— 牌面符号几何主机测试。
// 校验:轮廓点表合法、归一化映射正确、逆位 y 镜像正确、
// 78 张牌(22 组大阿卡纳构图 + 4 组花色符号派生)的每条原语都落在界内。
#include <assert.h>
#include <stdio.h>

#include "tarot_deck.h"
#include "tarot_emblem_math.h"

static void check_spec(const tarot_prim_spec_t *specs, int count) {
    assert(specs != NULL);
    assert(count >= 1 && count <= 8);
    for (int i = 0; i < count; i++) {
        const tarot_prim_spec_t *s = &specs[i];
        assert(s->kind < TAROT_PRIM_KIND_COUNT);
        assert(s->role < TAROT_ROLE_COUNT);
        assert(s->nw > 0 && s->nh > 0);
        assert(s->nx <= TAROT_EMBLEM_NORM);
        assert(s->ny <= TAROT_EMBLEM_NORM);
        assert((int)s->nx + (int)s->nw <= TAROT_EMBLEM_NORM);
        assert((int)s->ny + (int)s->nh <= TAROT_EMBLEM_NORM);
        if (s->kind == TAROT_PRIM_POLY) {
            assert(tarot_poly_points((int)s->aux, NULL, NULL) > 0);
        } else if (s->kind == TAROT_PRIM_ARC) {
            assert(s->aux < s->aux2);
            assert(s->aux2 <= 360);
        } else {
            assert(s->aux == 0 && s->aux2 == 0);
        }
    }
}

int main(void) {
    // 轮廓点表。
    assert(tarot_poly_points(0, NULL, NULL) == 3);    // 三角
    assert(tarot_poly_points(3, NULL, NULL) == 10);   // 五角星
    assert(tarot_poly_points(6, NULL, NULL) == 12);   // 十字
    assert(tarot_poly_points(-1, NULL, NULL) == 0);
    assert(tarot_poly_points(7, NULL, NULL) == 0);
    const uint16_t *xs, *ys;
    int n = tarot_poly_points(3, &xs, &ys);
    assert(xs && ys);
    for (int i = 0; i < n; i++) {
        assert(xs[i] <= TAROT_EMBLEM_NORM && ys[i] <= TAROT_EMBLEM_NORM);
    }

    // 归一化映射与逆位镜像。
    int x, y, w, h;
    tarot_prim_spec_t spec = {TAROT_PRIM_BAR, TAROT_ROLE_GOLD, 0,
                              0, 0, 1000, 500, 0};
    tarot_prim_place(&spec, 200, 100, false, &x, &y, &w, &h);
    assert(x == 0 && y == 0 && w == 200 && h == 50);
    tarot_prim_place(&spec, 200, 100, true, &x, &y, &w, &h);
    assert(x == 0 && y == 50 && w == 200 && h == 50);
    spec.nx = 250;
    spec.ny = 250;
    spec.nw = 500;
    spec.nh = 500;
    tarot_prim_place(&spec, 100, 200, false, &x, &y, &w, &h);
    assert(x == 25 && y == 50 && w == 50 && h == 100);
    tarot_prim_place(&spec, 100, 200, true, &x, &y, &w, &h);
    assert(x == 25 && y == 50 && w == 50 && h == 100);
    // 极小规格 clamp 到 1px。
    spec.nw = 1;
    spec.nh = 1;
    tarot_prim_place(&spec, 100, 100, false, &x, &y, &w, &h);
    assert(w == 1 && h == 1);
    // 退化输入安全。
    tarot_prim_place(NULL, 100, 100, false, &x, &y, &w, &h);
    assert(w == 0 && h == 0);
    tarot_prim_place(&spec, 0, 100, false, &x, &y, &w, &h);
    assert(w == 0 && h == 0);

    // 大阿卡纳:22 组构图逐条合法。
    for (int id = 0; id < 22; id++) {
        const tarot_prim_spec_t *specs = NULL;
        int count = tarot_major_spec(id, &specs);
        check_spec(specs, count);
    }
    assert(tarot_major_spec(-1, NULL) == 0);
    assert(tarot_major_spec(22, NULL) == 0);

    // 花色符号:4 组构图逐条合法。
    for (int suit = 0; suit < 4; suit++) {
        const tarot_prim_spec_t *specs = NULL;
        int count = tarot_suit_spec(suit, &specs);
        check_spec(specs, count);
    }
    assert(tarot_suit_spec(-1, NULL) == 0);
    assert(tarot_suit_spec(4, NULL) == 0);

    // 数字牌点阵:rank 张数正确、坐标界内、rank 9/10 复用 8 柱点阵。
    for (int rank = 1; rank <= 10; rank++) {
        const uint16_t *pxs = NULL, *pys = NULL;
        int count = tarot_pip_layout(rank, &pxs, &pys);
        assert(count == rank && pxs && pys);
        for (int i = 0; i < count; i++) {
            assert(pxs[i] <= TAROT_EMBLEM_NORM);
            assert(pys[i] <= TAROT_EMBLEM_NORM);
        }
    }
    const uint16_t *r8x, *r8y, *r10x, *r10y;
    assert(tarot_pip_layout(8, &r8x, &r8y) == 8);
    assert(tarot_pip_layout(10, &r10x, &r10y) == 10);
    for (int i = 0; i < 8; i++) {
        assert(r8x[i] == r10x[i] && r8y[i] == r10y[i]);
    }
    assert(tarot_pip_layout(0, NULL, NULL) == 0);
    assert(tarot_pip_layout(11, NULL, NULL) == 0);

    // 宫廷牌布局。
    uint16_t cnx, cny, cnw, cnh;
    int insignia = -1;
    for (int rank = 11; rank <= 14; rank++) {
        assert(tarot_court_layout(rank, &cnx, &cny, &cnw, &cnh, &insignia));
        assert(cnw > 0 && cnh > 0);
        assert((int)cnx + (int)cnw <= TAROT_EMBLEM_NORM);
        assert((int)cny + (int)cnh <= TAROT_EMBLEM_NORM);
        assert(tarot_poly_points(insignia, NULL, NULL) > 0);
    }
    assert(!tarot_court_layout(10, NULL, NULL, NULL, NULL, NULL));
    assert(!tarot_court_layout(15, NULL, NULL, NULL, NULL, NULL));

    printf("emblem_math: polys/place/78-card specs/pips/courts PASS\n");
    return 0;
}
