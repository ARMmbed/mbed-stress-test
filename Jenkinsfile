// Jenkins configuration.
properties ([
    buildDiscarder(
        logRotator(artifactDaysToKeepStr: '',
                   artifactNumToKeepStr: '',
                   daysToKeepStr: '30',
                   numToKeepStr: '100'
        )
    )
])

// RaaS credentials and settings.
raas_username = "ci"
raas_password = "ci"
raas_timeout = 1200

// Test combinations, for each listed target, each toolchain is build and tested on RaaS instance.
targets = [
    "K64F":           ["toolchains": [ "ARM", "IAR", "GCC_ARM"], "raas": "eeva.mbedcloudtesting.com"],
//    "NUCLEO_F429ZI":  ["toolchains": [ "ARM", "IAR", "GCC_ARM"], "raas": "ruka.mbedcloudtesting.com"],
]

// Map toolchains to compiler labels to find suitable node on Jenkins.
def nodes = [
    ARM: "armcc",
    IAR: "iar_arm",
    GCC_ARM: "arm-none-eabi-gcc",
    SXOS: "linux"
]

// Initial maps for parallel build steps
def buildStepsForParallel = [:]
def testStepsForParallel = [:]

for (target in targets.keySet()) {
    for (toolchain_key in targets[target]["toolchains"]) {
        echo "Include for build: ${target} ${toolchain_key}"

        def stepName = "${target} ${toolchain_key}"
        buildStepsForParallel[stepName] = buildStep(target, nodes[toolchain_key], toolchain_key)
    }
}

for (target in targets.keySet()) {
    for (toolchain_key in targets[target]["toolchains"]) {
        echo "Include for test: ${target} ${toolchain_key}"

        def stepName = "${target} ${toolchain_key}"
        testStepsForParallel[stepName] = testStep(target, nodes[toolchain_key], toolchain_key)
    }
}

// Actually run the steps in parallel - parallel takes a map as an argument, hence the above.
timestamps {
    parallel buildStepsForParallel
    parallel testStepsForParallel
}

// Create build steps for parallel execution.
def buildStep(target, compilerLabel, toolchain) {
    return {
        stage("Build_${target}_${toolchain}") {
            node("${compilerLabel}") {
                deleteDir()

                echo "buildStep: ${target} ${compilerLabel} ${toolchain}"
                dir("test") {

                    // checkout PR.
                    checkout scm

                    // checkout project.
                    sh "mbed deploy"

                    // build tests
                    sh "mbed test -vv --compile -m ${target} -t ${toolchain} -n '*stress*' --build ci --stats-depth 10 --profile release"

                    // stash build directory for testins step.
                    stash name: "mbed-stress-test-${target}-${toolchain}", includes: "ci/**"
                }

                step([$class: 'WsCleanup'])
            }
        }
    }
}

def testStep(target, compilerLabel, toolchain) {
    return {
        stage("Light suite ${target}_${toolchain}") {
            node("${compilerLabel}") {
                deleteDir()

                echo "testStep: ${target} ${toolchain}"

                // unstash build directory containing greentea tests.
                unstash "mbed-stress-test-${target}-${toolchain}"

                // setup RaaS environment.
                env.RAAS_USERNAME = raas_username
                env.RAAS_PASSWORD = raas_password
                env.RAAS_PYCLIENT_FORCE_REMOTE_ALLOCATION = 1
                env.RAAS_PYCLIENT_ALLOCATION_QUEUE_TIMEOUT = raas_timeout

                raas = targets[target]["raas"]

                // execute greentea on RaaS.
                execute("mbedgt --grm ${target}:raas_client:${raas}:80 -vV --test-spec ./ci/test_spec.json --polling-timeout 240 --tag-filters HAS_SD_CARD")

                // Clean up workarea.
                step([$class: 'WsCleanup'])
            }
        }
    }
}
