import org.jetbrains.kotlin.gradle.ExperimentalKotlinGradlePluginApi
import org.jetbrains.kotlin.gradle.dsl.JvmTarget

plugins {
    kotlin("multiplatform")
    kotlin("plugin.compose")
    id("com.android.application")
    id("org.jetbrains.compose")
}

kotlin {
    androidTarget {
        @OptIn(ExperimentalKotlinGradlePluginApi::class)
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_11)
        }
    }

    // iOS targets
    val iosTargets = listOf(
        iosX64(),
        iosArm64(),
        iosSimulatorArm64(),
    )

    iosTargets.forEach { iosTarget ->
        iosTarget.binaries.framework {
            baseName = "ComposeApp"
            isStatic = true
        }

        // C interop for MDK SDK
        iosTarget.compilations.getByName("main").cinterops {
            val mdk by creating {
                defFile(project.file("nativeInterop/cinterop/mdk.def"))
                packageName("mdk.cinterop")
                // MDK SDK path resolution:
                //  1. mdk.sdk.ios gradle property
                //  2. MDK_SDK_IOS env var
                //  3. MDK_SDK_PATH env var
                //  4. Default: libs/mdk-sdk inside this module
                val sdkPath = (project.findProperty("mdk.sdk.ios") as String?)
                    ?: System.getenv("MDK_SDK_IOS")
                    ?: System.getenv("MDK_SDK_PATH")
                    ?: "${projectDir}/libs/mdk-sdk"
                includeDirs("$sdkPath/include")
            }
        }
    }

    jvm("desktop")

    sourceSets {
        commonMain.dependencies {
            implementation(compose.runtime)
            implementation(compose.foundation)
            implementation(compose.material3)
            implementation(compose.ui)
            implementation(compose.components.resources)
        }

        androidMain.dependencies {
            implementation(compose.preview)
            implementation("androidx.activity:activity-compose:1.9.3")
        }

        // iosMain has no extra Kotlin dependencies; MDK is linked via cinterop.

        val desktopMain by getting {
            dependencies {
                implementation(compose.desktop.currentOs)
                // JNA for loading the pre-built mdk_shim native library
                implementation("net.java.dev.jna:jna:5.14.0")
                implementation("net.java.dev.jna:jna-platform:5.14.0")
            }
        }
    }
}

android {
    namespace = "org.mdk.example"
    compileSdk = 35

    defaultConfig {
        applicationId = "org.mdk.example"
        minSdk = 21
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments("-DANDROID_STL=c++_shared")
            }
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/androidMain/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    buildFeatures {
        compose = true
    }
}

compose.desktop {
    application {
        mainClass = "org.mdk.example.MainKt"
    }
}
