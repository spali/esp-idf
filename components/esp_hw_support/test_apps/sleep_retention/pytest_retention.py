# SPDX-FileCopyrightText: 2022-2026 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0
import functools
from typing import Callable
from typing import Dict
from typing import List

import pytest
from pytest_embedded import Dut
from pytest_embedded_idf.utils import idf_parametrize


def target_list(targets: List[str]) -> Callable:
    def decorator(func: Callable) -> Callable:
        @functools.wraps(func)
        def wrapper(*args: List, **kwargs: Dict) -> Callable:
            return func(*args, **kwargs)  # type: ignore

        for target in targets:
            wrapper = pytest.mark.__getattr__(target)(wrapper)

        return wrapper

    return decorator


# SOC_PAU_SUPPORTED == 1
retention_targets = ['esp32c6', 'esp32h2', 'esp32p4', 'esp32c5', 'esp32c61']


@target_list(retention_targets)
@pytest.mark.generic
@idf_parametrize('target', ['esp32c61', 'esp32c5', 'esp32p4', 'esp32h2', 'esp32c6'], indirect=['target'])
def test_sleep_retention(dut: Dut) -> None:
    dut.run_all_single_board_cases()
