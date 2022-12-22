// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ArenaBattle.h"
#include "GameFramework/Actor.h"
#include "Fountain.generated.h"


UCLASS()
class ARENABATTLE_API AFountain : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFountain();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	//virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Body;

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Water;

	UPROPERTY(VisibleAnywhere)
	UPointLightComponent* Light;

	UPROPERTY(VisibleAnywhere)
	UParticleSystemComponent* Splash;
	
	UPROPERTY(EditAnywhere, Category = ID)
	int32 ID;

public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;

private:
	// 은닉을 고려한 설계방식
	// UPROPERTY메크로에 AllowPrivateAccess라는 META키워드 추가하면 에디터에서는 이를 편집함과 동시에 변수 데이터를 은닉가능 => 캡슐화
	UPROPERTY(EditAnywhere, Category = Stat, Meta = (AllowPrivateAccess = true))
	float RotateSpeed = 30.f;

	// Movement Comp 테스트
public:
	UPROPERTY(VisibleAnywhere)
	class URotatingMovementComponent* Movement;
};
