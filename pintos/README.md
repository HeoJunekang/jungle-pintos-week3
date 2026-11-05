# ⚙️ Pintos 구현·설계 요약

**Project 3 · Virtual Memory**

사용자 가상주소의 페이지 정보를 관리하고, 페이지 폴트에 따라 필요한 데이터를 메모리에 준비하는 커널을 구현합니다. 프로젝트 소개·참여자·협업 방식은 [상위 README](../README.md)에 정리했습니다.

## 📋 개발 범위

| 기능 | 구현 내용 | 검증 상태 |
| --- | --- | --- |
| SPT | 가상 페이지 주소를 키로 하는 해시 테이블, 조회·등록·삭제 | 관련 실행 경로 구현 |
| Lazy Loading | UNINIT에 초기화 정보를 보관하고 첫 접근 시 내용 준비 | `lazy-file`, `lazy-anon` 통과 |
| Stack Growth | 사용자 스택 범위와 사용자 `rsp`로 확장 판단 | 스택·보호 관련 테스트 8/8 통과 |
| SPT Copy | UNINIT 초기화 정보 복사, ANON·FILE 페이지 복제 경로 | 기본 fork 테스트 통과. 메모리 압박 경로는 추가 점검 필요 |
| mmap / munmap | 파일 구간 등록·지연 로딩·매핑 단위 해제·수정 내용 반영 | mmap 테스트 25/25 통과 |
| Page Replacement | 전역 프레임 목록과 Clock 방식의 교체 대상 선택 | 메모리 압박 테스트에서 실패 남음 |
| ANON Swap | 비트맵으로 슬롯 관리, 스왑 디스크에 페이지 보존·복원 | `swap-fork` 통과, 스왑 3개 실패 |
| FILE Swap | 파일 위치로 재로딩, dirty 페이지의 유효 바이트 반영 | 경로 구현. `swap-file` 실패 |
| Copy-on-Write · 추가 과제 | 쓰기 보호 폴트 처리 함수가 비어 있음 | 미구현, `cow-simple` 실패 |

## 🧩 핵심 구현

### 페이지 등록과 실제 로딩 분리

- `vm_alloc_page_with_initializer()`에서 페이지 종류·권한·초기화 정보를 SPT에 등록합니다.
- 첫 접근에서 페이지 폴트가 발생하면 페이지를 찾고, claim 경로에서 프레임·매핑·내용을 준비합니다.
- 실제 프레임을 확보하기 전의 관리 정보와 RAM에 올라간 데이터의 상태를 구분합니다.

### 사용자 스택과 시스템 콜 연결

- `setup_stack()`은 첫 사용자 스택 페이지와 초기 `rsp`를 준비합니다.
- 시스템 콜 진입 시 사용자 `rsp`를 스레드에 보관합니다.
- 커널에서 사용자 버퍼에 접근하다 발생한 폴트는 저장한 사용자 `rsp`를 확장 판단에 사용합니다.

### 파일 매핑과 자원 수명

- `file_reopen()`으로 매핑용 파일 객체를 확보합니다.
- 페이지마다 파일 오프셋과 읽을 바이트 수를 보관하고, 파일 내용 이후의 공간은 0으로 채웁니다.
- `mmap_info`로 한 매핑의 시작 주소·페이지 수·파일 객체를 묶고, `munmap`에서 해당 범위를 정리합니다.

### 프레임 교체와 페이지별 복원

- 프레임 목록과 Accessed 비트를 이용해 교체 대상을 선택합니다.
- ANON은 4 KiB 내용을 512바이트 섹터 8개에 저장하고, 슬롯 번호로 복원합니다.
- FILE은 연결된 파일에서 다시 읽습니다. dirty 상태이면 파일에 유효 데이터만 반영합니다.

## 🧭 설계의 핵심

| 구분 | 설계 의도 |
| --- | --- |
| SPT / PML4 / Frame | 페이지 관리 정보, 현재 주소 매핑, 실제 저장 공간의 역할 분리 |
| UNINIT / ANON / FILE | 초기화·스왑·정리 동작을 페이지 타입별 operations로 연결 |
| 사용자 rsp 보관 | 현재 커널 실행 문맥과 스택 확장에 필요한 사용자 문맥 구분 |
| 매핑용 파일 객체 | 원래 fd가 닫혀도 매핑의 파일 접근 수명을 유지 |

## 🔎 대표 트러블슈팅

**증상:** 일반 스택 확장은 동작하지만 `pt-grow-stk-sc`에서 시스템 콜의 사용자 스택 버퍼 접근이 실패했습니다.

**원인:** 커널 모드 폴트의 `f->rsp`를 사용자 스택 위치로 해석했고, 버퍼 검사는 정상적인 미매핑 스택 후보도 실제 접근 전에 종료시켰습니다.

**수정:** 시스템 콜 진입 시 사용자 `rsp`를 보관하고, 폴트 발생 모드에 따라 판단 기준을 선택했습니다. 버퍼 검사도 정상적인 스택 확장 후보를 구분하도록 수정했습니다. 이번 검증에서 해당 테스트는 **PASS**입니다.

## 🧪 테스트 결과

**검증일: 2026-11-05 · 코드 기준: `810e082`**

기존 빌드·결과 파일이 없는 복사본에서 VM 설정의 전체 `make check`를 실행했습니다.

| 테스트 묶음 | PASS | FAIL |
| --- | ---: | ---: |
| User Programs | 63 | 0 |
| VM · 기본 | 39 | 7 |
| File Systems · base | 13 | 0 |
| Threads · Alarm / Priority | 18 | 0 |
| COW · 추가 과제 | 0 | 1 |
| **전체** | **133** | **8** |

- **실패:** `page-merge-seq`, `page-merge-par`, `page-merge-stk`, `page-merge-mm`, `swap-file`, `swap-anon`, `swap-iter`, `cow/cow-simple`.
- **확인된 한계:** 페이지 병합·스왑에서 커널 패닉이 발생합니다. 프레임 교체 시 소유 주소 공간 처리·연결 해제 순서, 스왑 슬롯과 페이지 정리 경로를 점검해야 합니다.
- 기본 fork와 mmap 통과가 메모리 압박·실패 경로의 완전성을 보장하지는 않습니다. MLFQS 전체 및 Project 4 확장 기능은 이번 실행 범위에 포함하지 않았습니다.

실행 조건과 실패 출력은 [검증 기록](../docs/test-results/2026-11-05-main-810e082.md)에 있습니다.

## 🛠️ 빌드와 검증

저장소 루트에서 연 **개발 컨테이너 터미널** 기준입니다.

```bash
source pintos/activate
cd pintos/vm
make -j4 check
```

기존 결과를 재사용할 수 있으므로 코드 변경 전후를 비교할 때는 새 빌드 공간을 사용하세요. 위 테스트 결과는 별도의 새 복사본에서 얻은 값입니다.

## 📂 주요 코드

| 경로 | 역할 |
| --- | --- |
| [vm/vm.c](vm/vm.c) | SPT·claim·스택 확장·프레임 교체 |
| [vm/uninit.c](vm/uninit.c) | 첫 접근 시 페이지 초기화 |
| [vm/anon.c](vm/anon.c) | 익명 페이지와 스왑 슬롯 |
| [vm/file.c](vm/file.c) | mmap·munmap·FILE 로딩과 반영 |
| [userprog/process.c](userprog/process.c) | 실행파일 로딩·초기 스택·프로세스 메모리 수명 |
| [userprog/syscall.c](userprog/syscall.c) | 사용자 rsp 보관·버퍼 검증·시스템 콜 |
| [include/vm/vm.h](include/vm/vm.h) | page·frame·SPT·operations 정의 |

## 📚 참고

원본: **Pintos for Operating Systems and Lab (CS330), KAIST, by Youngjin Kwon.**

[KAIST Pintos 공식 문서](https://casys-kaist.github.io/pintos-kaist/) · [원본 라이선스](LICENSE)
