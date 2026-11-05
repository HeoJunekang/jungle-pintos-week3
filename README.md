# 🧠 Pintos · Virtual Memory

**KAIST Pintos를 기반으로 가상 메모리 관리 기능을 구현한 운영체제 학습 프로젝트입니다.**

프로세스의 가상 페이지를 관리하고, 실제 접근 시 필요한 데이터를 메모리에 준비하며, 메모리가 부족하면 기존 프레임을 교체하는 흐름을 구현합니다. 이전 단계의 스레드·사용자 프로그램 기능을 바탕으로 **지연 로딩, 스택 확장, 파일 매핑, 스왑**을 연결했습니다.

[구현·설계 요약](pintos/README.md) · [협업 가이드](.github/CONTRIBUTING.md) · [검증 기록](docs/test-results/2026-11-05-main-810e082.md) · [KAIST 공식 문서](https://casys-kaist.github.io/pintos-kaist/)

## 📍 현재 상태

> **가상 메모리 주요 기능 구현 · 메모리 교체 경로 검증 진행 중**
>
> `main`의 코드 기준 커밋 `810e082`를 새로 빌드·테스트했습니다. **기본 테스트 133/140 통과**, 추가 과제인 **COW 0/1 통과**입니다. 기본 테스트의 남은 실패는 페이지 병합 4개와 스왑 3개입니다.

| 개발 범위 | 핵심 내용 | 현재 확인 범위 |
| --- | --- | --- |
| Threads | Alarm Clock, 우선순위 스케줄링·Donation, 동기화 | VM 빌드에 포함된 관련 테스트 18개 통과 |
| User Programs | 인자 전달, 프로세스 관리, 시스템 콜 | VM 빌드에 포함된 관련 테스트 63개 통과 |
| **Virtual Memory** | **SPT, 지연 로딩, 스택 확장, mmap·munmap, 스왑** | **기본 VM 테스트 39/46 통과** |
| File Systems | 제공된 기본 파일 시스템 활용 | 기본 회귀 테스트 13개 통과. Project 4 확장 기능 완료를 뜻하지 않음 |

## ✨ 주요 구현

- **필요한 순간에 로딩:** 페이지 정보를 먼저 등록하고, 첫 접근에서 프레임과 내용을 준비합니다.
- **사용자 스택 확장:** 접근 주소와 사용자 `rsp`를 기준으로 정상적인 스택 접근을 판단합니다.
- **파일 매핑:** `mmap`으로 파일 구간을 가상주소에 연결하고, `munmap`에서 매핑을 정리합니다.
- **메모리 교체:** 프레임의 접근 흔적으로 교체 대상을 선택하고, ANON과 FILE에 맞는 보존·복원 경로를 구성했습니다. 이 경로에는 실패 테스트가 남아 있습니다.

## 👥 VM 개발 참여

| 이름·활동명 | GitHub |
| --- | --- |
| 허준강 | [@HeoJunekang](https://github.com/HeoJunekang) |
| stereokkh | [@stereokkh](https://github.com/stereokkh) |
| 김명준 | [@haihai1222555-droid](https://github.com/haihai1222555-droid) |

VM 개발 기간의 커밋 작성자 기준입니다. 이전 프로젝트의 코드와 기여 이력도 저장소에 포함되어 있습니다.

## 🤝 협업 방식

- 개인 작업 브랜치에서 기능을 구현하고, PR로 `develop`에 통합합니다.
- 프로젝트 단위로 검토한 내용을 `main`에 반영합니다.
- PR에는 변경 목적·설계·검증 결과를 기록하고, 관련 테스트와 기존 기능의 회귀 여부를 확인합니다.
- 커밋은 `feat`, `fix`, `refactor`, `test`, `docs`, `chore`로 작업 목적을 구분합니다.

세부 규칙은 [협업 가이드](.github/CONTRIBUTING.md)와 [PR 템플릿](.github/pull_request_template.md)을 따릅니다.

## 🧪 검증 요약

**2026-11-05 · `810e082` · Ubuntu 22.04 컨테이너 / GCC / QEMU**

| 항목 | 결과 |
| --- | --- |
| 새 VM 커널 빌드 | 성공, 컴파일 경고 남음 |
| 기본 테스트 | **133 PASS / 7 FAIL** |
| 추가 COW 테스트 | **0 PASS / 1 FAIL** |

`pt-grow-stk-sc`, mmap 테스트 25개, 지연 로딩 테스트 2개가 통과했습니다. 메모리 압박 상황의 페이지 병합·스왑에서는 커널 패닉이 발생했고, COW는 미구현 상태입니다. 상세 결과와 실행 조건은 [검증 기록](docs/test-results/2026-11-05-main-810e082.md)에 정리했습니다.

## 🛠️ 개발 환경

**C · x86-64 · Ubuntu 22.04 · Docker / VS Code Dev Containers · QEMU · GDB**

Docker 설치부터 VS Code 개발 컨테이너 구성까지는 [Docker 기반 개발 환경 구축 가이드](docs/SETUP.md)를 참고하세요. 기존 환경 구축 가이드 원문을 별도 보존했습니다.

개발 컨테이너 설정은 [`.devcontainer`](.devcontainer)에 있습니다. 빌드·테스트 방법은 [내부 README](pintos/README.md#-빌드와-검증)를 참고하세요.

## 📚 기반 프로젝트

[KAIST Pintos](https://casys-kaist.github.io/pintos-kaist/)는 KAIST CS330 운영체제 실습용 프로젝트입니다. 이 저장소는 해당 코드와 [KRAFTON Jungle 개발 환경](https://github.com/krafton-jungle/pintos_22.04_lab_docker)을 기반으로 합니다. 원본 라이선스는 [pintos/LICENSE](pintos/LICENSE)에 보존되어 있습니다.
